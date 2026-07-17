import type { Point } from "./types";
import { distance } from "../world/WorldMap";

export type NoiseKind = "step" | "sprint" | "bolt" | "door" | "impact" | "trolley" | "alarm";
export interface NoiseEvent {
  id: number;
  kind: NoiseKind;
  position: Point;
  radius: number;
  strength: number;
  expiresAt: number;
}

export class NoiseSystem {
  private events: NoiseEvent[] = [];
  private nextId = 1;

  emit(kind: NoiseKind, position: Point, radius: number, durationMs = 1800, strength = 1): NoiseEvent {
    const event = { id: this.nextId++, kind, position: { ...position }, radius, strength, expiresAt: performance.now() + durationMs };
    this.events.push(event);
    return event;
  }

  update(now = performance.now()): void { this.events = this.events.filter((event) => event.expiresAt > now); }
  clear(): void { this.events.length = 0; }
  count(): number { return this.events.length; }
  snapshot(): readonly NoiseEvent[] { return this.events.map((event) => ({ ...event, position: { ...event.position } })); }

  strongestFor(listener: Point, hearingRadius: number, now = performance.now()): NoiseEvent | null {
    this.update(now);
    let best: NoiseEvent | null = null;
    let bestScore = 0;
    for (const event of this.events) {
      const range = Math.min(hearingRadius, event.radius);
      const d = distance(listener, event.position);
      if (d > range) continue;
      const distractionPriority = event.kind === "bolt" ? 2.5 : 1;
      const score = event.strength * distractionPriority * (1 - d / Math.max(1, range));
      if (score > bestScore) { best = event; bestScore = score; }
    }
    return best;
  }
}
