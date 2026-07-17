import type { DroneSpec, DroneState, Point } from "../game/types";
import type { NoiseEvent } from "../game/noise";
import { distance } from "../world/WorldMap";
import { findWorldPath } from "../game/pathfinding";

export interface DroneDecision {
  velocity: Point;
  state: DroneState;
  warning: boolean;
  stateChanged: boolean;
}

export class DroneController {
  state: DroneState;
  private routeIndex = 0;
  private target: Point | null = null;
  private lastKnown: Point | null = null;
  private path: Point[] = [];
  private pathIndex = 0;
  private recalcMs = 0;
  private alertMs = 0;
  private searchMs = 0;
  private stunnedMs = 0;
  private warningCooldownMs = 0;
  private patrolVisits = 0;

  constructor(readonly spec: DroneSpec) { this.state = spec.activateOnLockdown ? "dormant" : "patrol"; }

  stun(durationMs: number): void { this.stunnedMs = Math.max(this.stunnedMs, durationMs); this.state = "stunned"; this.path = []; }
  activate(): void { if (this.state === "dormant") this.state = "patrol"; }
  targetPosition(): Point | null { return this.target ? { ...this.target } : null; }

  update(
    deltaMs: number,
    position: Point,
    player: Point,
    blocked: ReadonlySet<string>,
    canSeePlayer: boolean,
    noise: NoiseEvent | null,
    lockdown: boolean,
  ): DroneDecision {
    const previous = this.state;
    this.warningCooldownMs = Math.max(0, this.warningCooldownMs - deltaMs);
    if (this.state === "dormant") return { velocity: { x: 0, y: 0 }, state: this.state, warning: false, stateChanged: false };
    if (this.stunnedMs > 0) {
      this.stunnedMs -= deltaMs;
      if (this.stunnedMs <= 0) this.state = "return";
      return { velocity: { x: 0, y: 0 }, state: this.state, warning: false, stateChanged: previous !== this.state };
    }

    if (canSeePlayer && distance(position, player) <= this.spec.sightRadius * (lockdown ? 1.15 : 1)) {
      this.state = "chase";
      this.target = { ...player };
      this.lastKnown = { ...player };
      this.alertMs = 3200;
    } else if (noise && this.state !== "chase") {
      this.state = "investigate";
      this.target = { ...noise.position };
      this.lastKnown = { ...noise.position };
      this.alertMs = 2600;
    } else if (this.state === "chase") {
      this.alertMs -= deltaMs;
      if (this.alertMs <= 0) { this.state = "search"; this.target = this.lastKnown; this.searchMs = 5200; }
    } else if (this.state === "investigate") {
      this.alertMs -= deltaMs;
      if (this.alertMs <= 0 || (this.target && distance(position, this.target) < 40)) { this.state = "search"; this.searchMs = 4200; }
    } else if (this.state === "search") {
      this.searchMs -= deltaMs;
      if (this.searchMs <= 0) { this.state = "return"; this.target = null; }
      else if (!this.target || distance(position, this.target) < 42) {
        const phase = Math.floor(this.searchMs / 900) % 4;
        const offsets: readonly Point[] = [{ x: 128, y: 0 }, { x: 0, y: 128 }, { x: -128, y: 0 }, { x: 0, y: -128 }];
        const base = this.lastKnown ?? position;
        const offset = offsets[phase] ?? offsets[0] ?? { x: 0, y: 0 };
        this.target = { x: base.x + offset.x, y: base.y + offset.y };
      }
    }

    if (this.state === "patrol" || this.state === "return") {
      const patrolTarget = this.spec.route[this.routeIndex] ?? this.spec.route[0] ?? position;
      this.target = patrolTarget;
      if (distance(position, patrolTarget) < 44) {
        this.patrolVisits += 1;
        const step = this.patrolVisits % 3 === 0 && this.spec.kind === "listener" ? 2 : 1;
        this.routeIndex = (this.routeIndex + step) % this.spec.route.length;
        this.target = this.spec.route[this.routeIndex] ?? this.spec.route[0] ?? position;
        if (this.state === "return") this.state = "patrol";
      }
    }

    this.recalcMs -= deltaMs;
    const goal = this.target ?? this.spec.route[this.routeIndex];
    if (goal && (this.recalcMs <= 0 || this.path.length === 0 || this.pathIndex >= this.path.length)) {
      this.path = findWorldPath(position, goal, blocked);
      this.pathIndex = this.path.length > 1 ? 1 : 0;
      this.recalcMs = this.state === "chase" ? 360 : 720;
    }
    while (this.pathIndex < this.path.length && distance(position, this.path[this.pathIndex] ?? position) < 30) this.pathIndex += 1;
    const waypoint = this.path[this.pathIndex] ?? goal;
    if (!waypoint) return { velocity: { x: 0, y: 0 }, state: this.state, warning: false, stateChanged: previous !== this.state };
    const angle = Math.atan2(waypoint.y - position.y, waypoint.x - position.x);
    const stateMultiplier = this.state === "chase" ? 1.32 : this.state === "investigate" ? 1.12 : 1;
    const kindMultiplier = this.spec.kind === "interceptor" ? 1.12 : 1;
    const speed = this.spec.speed * stateMultiplier * kindMultiplier * (lockdown ? 1.12 : 1);
    const warning = this.state === "chase" && distance(position, player) < 210 && this.warningCooldownMs === 0;
    if (warning) this.warningCooldownMs = 2200;
    return {
      velocity: { x: Math.cos(angle) * speed, y: Math.sin(angle) * speed },
      state: this.state,
      warning,
      stateChanged: previous !== this.state,
    };
  }
}
