import { AudioGameEngine, type BuiltInRoom, type SoundHandle, type Vec3 } from "@aaja/audio-game-engine";
import { GameDiagnostics } from "../diagnostics/Diagnostics";
import type { Point } from "../game/types";
import { WORLD } from "../world/WorldMap";

export type SoundName =
  | "footstep-wood" | "footstep-metal" | "footstep-gravel" | "footstep-concrete" | "footstep-grating" | "footstep-rubber"
  | "door-open" | "door-close" | "gate" | "switch" | "trolley-loop"
  | "drone-sentinel-loop" | "drone-listener-loop" | "drone-interceptor-loop"
  | "alarm" | "pickup" | "drop" | "impact" | "cooling-loop" | "bolt" | "success" | "failure"
  | "core-hum-loop" | "core-overheat" | "lift-loop"
  | "yard-ambience" | "hangar-ambience" | "corridor-ambience" | "cooling-ambience" | "shaft-ambience" | "machine-ambience";

interface LoopOptions { category?: string; priority?: number; volume?: number; occlusion?: number; roomAmount?: number; }
interface LoopRecord { handle: SoundHandle; name: SoundName; }
interface LoopDefinition { name: SoundName; position: Point; options: LoopOptions; }

export class AajaAudioAdapter {
  private engine: AudioGameEngine | null = null;
  private readonly loops = new Map<string, LoopRecord>();
  private readonly pendingLoops = new Map<string, Promise<void>>();
  private readonly loopDefinitions = new Map<string, LoopDefinition>();
  private muted = false;

  constructor(private readonly diagnostics: GameDiagnostics, private readonly masterVolume: () => number) {}

  async start(): Promise<void> {
    if (this.engine) { await this.engine.resume(); return; }
    try {
      this.engine = await AudioGameEngine.start({ quality: "hrtf", maxVoices: 32, masterVolume: this.masterVolume(), autoRecover: true });
      this.engine.configureCategory("danger", { volume: 0.95, priority: 100, maxVoices: 7 });
      this.engine.configureCategory("speech", { volume: 1, priority: 95, maxVoices: 1 });
      this.engine.configureCategory("mechanisms", { volume: 0.72, priority: 62, maxVoices: 9 });
      this.engine.configureCategory("environment", { volume: 0.55, priority: 25, maxVoices: 8 });
      this.engine.configureCategory("footsteps", { volume: 0.72, priority: 48, maxVoices: 4 });
      this.engine.configureCategory("drones", { volume: 0.74, priority: 72, maxVoices: 7 });
      this.engine.configureCategory("ui", { volume: 0.9, priority: 90, maxVoices: 3 });
      this.diagnostics.log("info", "audio.started", `Aaja ${this.engine.coreVersion} started`);
    } catch (error) {
      this.diagnostics.log("error", "audio.start", error instanceof Error ? error.message : String(error));
      throw error;
    }
  }

  isStarted(): boolean { return this.engine !== null; }
  activeSourceCount(): number { return this.engine?.activeSourceCount ?? 0; }

  setListener(position: Point, angle: number): void {
    if (!this.engine || this.muted) return;
    const p = this.toAudio(position);
    const forward: Vec3 = [Math.cos(angle), 0, Math.sin(angle)];
    this.engine.setListenerPosition(p);
    this.engine.setListenerOrientation(forward, [0, 1, 0]);
  }

  setRoom(room: BuiltInRoom): void { if (this.engine && !this.muted) this.engine.setRoom(room, 650); }

  async oneShot(name: SoundName, position: Point, options: { category?: string; priority?: number; volume?: number; occlusion?: number; roomAmount?: number } = {}): Promise<void> {
    if (!this.engine || this.muted) return;
    try {
      const handle = await this.engine.play(this.url(name), {
        position: this.toAudio(position),
        category: options.category ?? "mechanisms",
        priority: options.priority ?? 50,
        volume: options.volume ?? 0.75,
        occlusion: options.occlusion ?? 0,
        roomAmount: options.roomAmount ?? 0.68,
      });
      window.setTimeout(() => handle.dispose(), Math.max(1300, (handle.duration || 1) * 1000 + 250));
    } catch (error) {
      this.diagnostics.log("warning", `audio.play.${name}`, error instanceof Error ? error.message : String(error));
    }
  }

  async ensureLoop(id: string, name: SoundName, position: Point, options: LoopOptions = {}): Promise<void> {
    this.loopDefinitions.set(id, { name, position: { ...position }, options: { ...options } });
    if (!this.engine || this.muted) return;
    const existing = this.loops.get(id);
    if (existing) {
      existing.handle.setPosition(this.toAudio(position), 100);
      existing.handle.setOcclusion(options.occlusion ?? 0, 160);
      if (options.volume !== undefined) existing.handle.setVolume(options.volume, 120);
      return;
    }
    const pending = this.pendingLoops.get(id);
    if (pending) { await pending; return; }
    const creation = this.createLoop(id, name, position, options);
    this.pendingLoops.set(id, creation);
    try { await creation; } finally { if (this.pendingLoops.get(id) === creation) this.pendingLoops.delete(id); }
  }

  updateLoop(id: string, position: Point, occlusion = 0, volume?: number): void {
    const definition = this.loopDefinitions.get(id);
    if (definition) {
      definition.position = { ...position };
      definition.options = { ...definition.options, occlusion, ...(volume === undefined ? {} : { volume }) };
    }
    const loop = this.loops.get(id);
    if (!loop) return;
    loop.handle.setPosition(this.toAudio(position), 90);
    loop.handle.setOcclusion(occlusion, 150);
    if (volume !== undefined) loop.handle.setVolume(volume, 120);
  }

  async stopLoop(id: string, fadeMs = 180): Promise<void> {
    this.loopDefinitions.delete(id);
    const pending = this.pendingLoops.get(id);
    if (pending) await pending.catch(() => undefined);
    const loop = this.loops.get(id);
    if (!loop) return;
    this.loops.delete(id);
    await loop.handle.stop(fadeMs);
    loop.handle.dispose();
  }

  setSpeechDucking(active: boolean): void {
    if (!this.engine) return;
    this.engine.configureCategory("environment", { volume: active ? 0.2 : 0.55 });
    this.engine.configureCategory("mechanisms", { volume: active ? 0.4 : 0.72 });
    this.engine.configureCategory("drones", { volume: active ? 0.48 : 0.74 });
  }

  async emergencyStop(): Promise<void> {
    if (!this.engine) return;
    this.muted = true;
    await Promise.allSettled(this.pendingLoops.values());
    await this.engine.stopAll(40);
    for (const loop of this.loops.values()) loop.handle.dispose();
    this.loops.clear();
    this.pendingLoops.clear();
  }

  async resumeAfterStop(): Promise<void> {
    if (!this.engine) return;
    await this.engine.resume();
    if (!this.muted) return;
    this.muted = false;
    const definitions = [...this.loopDefinitions.entries()];
    await Promise.all(definitions.map(([id, definition]) => this.ensureLoop(id, definition.name, definition.position, definition.options)));
  }

  async dispose(): Promise<void> {
    this.loopDefinitions.clear();
    this.pendingLoops.clear();
    for (const loop of this.loops.values()) loop.handle.dispose();
    this.loops.clear();
    if (this.engine) await this.engine.close();
    this.engine = null;
  }

  private async createLoop(id: string, name: SoundName, position: Point, options: LoopOptions): Promise<void> {
    if (!this.engine || this.muted || this.loops.has(id)) return;
    try {
      const handle = await this.engine.play(this.url(name), {
        position: this.toAudio(position),
        loop: true,
        category: options.category ?? "environment",
        priority: options.priority ?? 35,
        volume: options.volume ?? 0.5,
        fadeInMs: 180,
        occlusion: options.occlusion ?? 0,
        roomAmount: options.roomAmount ?? 0.72,
      });
      if (this.muted || !this.loopDefinitions.has(id)) { await handle.stop(0); handle.dispose(); return; }
      this.loops.set(id, { handle, name });
    } catch (error) {
      this.diagnostics.log("warning", `audio.loop.${name}`, error instanceof Error ? error.message : String(error));
    }
  }

  private url(name: SoundName): string { return `${import.meta.env.BASE_URL}assets/audio/${name}.ogg`; }
  private toAudio(point: Point): Vec3 {
    return [(point.x - WORLD.width / 2) / WORLD.tileSize, 0, (point.y - WORLD.height / 2) / WORLD.tileSize] as const;
  }
}
