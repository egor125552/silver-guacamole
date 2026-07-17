import type { BuiltInRoom } from "@aaja/audio-game-engine";
import type { Point, Rect, ZoneSpec } from "../game/types";
import { WORLD, lineIntersectsRect, zoneAt } from "../world/WorldMap";
import { AajaAudioAdapter, type SoundName } from "./AajaAudioAdapter";

export class AudioScene {
  private lastZone = "";
  private dynamicBlockers: Rect[] = [];
  private activeAmbience = "";

  constructor(private readonly audio: AajaAudioAdapter) {}

  reset(): void {
    this.lastZone = "";
    this.dynamicBlockers = [];
    this.activeAmbience = "";
  }

  shutdown(): void {
    if (this.activeAmbience) void this.audio.stopLoop(this.activeAmbience, 0);
    this.activeAmbience = "";
  }

  setDynamicBlockers(rects: readonly Rect[]): void { this.dynamicBlockers = rects.map((rect) => ({ ...rect })); }

  updateListener(position: Point, angle: number): void {
    this.audio.setListener(position, angle);
    const zoneName = zoneAt(position);
    if (zoneName === this.lastZone) return;
    this.lastZone = zoneName;
    const zone = WORLD.zones.find((item) => item.name === zoneName) ?? WORLD.zones[0];
    if (!zone) return;
    this.audio.setRoom(zone.room as BuiltInRoom);
    void this.crossfadeAmbience(zone);
  }

  occlusion(listener: Point, source: Point): number {
    const blockers = [...WORLD.walls, ...this.dynamicBlockers];
    const hits = blockers.reduce((count, wall) => count + (lineIntersectsRect(listener, source, wall) ? 1 : 0), 0);
    const listenerZone = WORLD.zones.find((item) => item.name === zoneAt(listener));
    const sourceZone = WORLD.zones.find((item) => item.name === zoneAt(source));
    const zonePenalty = listenerZone?.name !== sourceZone?.name ? 0.14 + ((listenerZone?.absorption ?? 0.2) + (sourceZone?.absorption ?? 0.2)) * 0.25 : 0;
    return Math.min(0.9, hits * 0.28 + zonePenalty);
  }

  private async crossfadeAmbience(zone: ZoneSpec): Promise<void> {
    const next = `ambience-${zone.name}`;
    if (this.activeAmbience && this.activeAmbience !== next) await this.audio.stopLoop(this.activeAmbience, 650);
    this.activeAmbience = next;
    await this.audio.ensureLoop(next, zone.ambience as SoundName, {
      x: zone.rect.x + zone.rect.width / 2,
      y: zone.rect.y + zone.rect.height / 2,
    }, { category: "environment", priority: 18, volume: 0.28, roomAmount: 0.8 });
  }
}
