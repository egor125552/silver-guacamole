import type { CoreId, DroneState, Point, SurfaceName } from "../game/types";
import { WORLD, zoneAt } from "../world/WorldMap";
import type { AajaAudioAdapter, SoundName } from "./AajaAudioAdapter";
import type { AudioScene } from "./AudioScene";

interface CoreAudioSource { id: string; position: Point; }
interface DroneAudioSource { id: string; kind: string; position: Point; state: DroneState; }

export class GameAudioController {
  private currentSurface: SurfaceName = "metal";
  private spatialAccumulatorMs = 0;

  constructor(private readonly audio: AajaAudioAdapter, private readonly scene: AudioScene) {}

  async start(trolley: Point, cores: readonly CoreAudioSource[], drones: readonly DroneAudioSource[]): Promise<void> {
    await this.audio.ensureLoop("trolley", "trolley-loop", trolley, { category: "mechanisms", priority: 62, volume: 0.46 });
    await this.audio.ensureLoop("lift", "lift-loop", WORLD.bay, { category: "mechanisms", priority: 44, volume: 0.32 });
    for (const point of WORLD.coolPads) await this.audio.ensureLoop(`cool-${point.x}-${point.y}`, "cooling-loop", point, { category: "environment", priority: 31, volume: 0.32 });
    for (const core of cores) await this.audio.ensureLoop(`core-${core.id}`, "core-hum-loop", core.position, { category: "mechanisms", priority: 56, volume: 0.42 });
    for (const drone of drones) {
      const name = `drone-${drone.kind}-loop` as SoundName;
      await this.audio.ensureLoop(`drone-${drone.id}`, name, drone.position, { category: "drones", priority: 72, volume: drone.state === "dormant" ? 0.05 : 0.46 });
    }
  }


  pickCore(id: CoreId): void { void this.audio.stopLoop(`core-${id}`, 120); }

  dropCore(id: CoreId, position: Point): void {
    void this.audio.ensureLoop(`core-${id}`, "core-hum-loop", position, { category: "mechanisms", priority: 56, volume: 0.42 });
  }

  deliverCore(id: CoreId): void { void this.audio.stopLoop(`core-${id}`, 180); }

  update(options: {
    deltaMs: number;
    player: Point;
    angle: number;
    moving: boolean;
    sprinting: boolean;
    carriedCore: boolean;
    coreHeat: number;
    trolley: Point;
    drones: readonly DroneAudioSource[];
  }): void {
    this.scene.updateListener(options.player, options.angle);
    const zone = WORLD.zones.find((item) => item.name === zoneAt(options.player));
    const surface = zone?.surface ?? "metal";
    if (surface !== this.currentSurface) {
      this.currentSurface = surface;
      void this.audio.stopLoop("player-footsteps", 100);
    }
    if (options.moving) {
      const name = `footstep-${surface}` as SoundName;
      const volume = options.sprinting ? 0.76 : 0.55;
      void this.audio.ensureLoop("player-footsteps", name, options.player, { category: "footsteps", priority: 52, volume });
      this.audio.updateLoop("player-footsteps", options.player, 0, volume);
    } else void this.audio.stopLoop("player-footsteps", 100);

    if (options.carriedCore) {
      const volume = 0.34 + options.coreHeat / 220;
      void this.audio.ensureLoop("carried-core", "core-hum-loop", options.player, { category: "mechanisms", priority: 66, volume });
      this.audio.updateLoop("carried-core", options.player, 0, volume);
    } else void this.audio.stopLoop("carried-core", 160);

    this.spatialAccumulatorMs += options.deltaMs;
    if (this.spatialAccumulatorMs < 110) return;
    this.spatialAccumulatorMs = 0;
    this.audio.updateLoop("trolley", options.trolley, this.scene.occlusion(options.player, options.trolley), 0.46);
    for (const drone of options.drones) {
      const volume = drone.state === "dormant" ? 0.05 : drone.state === "stunned" ? 0.16 : 0.46;
      this.audio.updateLoop(`drone-${drone.id}`, drone.position, this.scene.occlusion(options.player, drone.position), volume);
    }
  }

  shutdown(droneIds: readonly string[]): void {
    for (const id of ["player-footsteps", "carried-core", "trolley", "lift"]) void this.audio.stopLoop(id, 0);
    for (const point of WORLD.coolPads) void this.audio.stopLoop(`cool-${point.x}-${point.y}`, 0);
    for (const core of WORLD.cores) void this.audio.stopLoop(`core-${core.id}`, 0);
    for (const id of droneIds) void this.audio.stopLoop(`drone-${id}`, 0);
    this.scene.shutdown();
  }
}
