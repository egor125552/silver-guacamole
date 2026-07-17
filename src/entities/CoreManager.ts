import Phaser from "phaser";
import type { CoreId, CoreSpec, Point } from "../game/types";
import { distance } from "../world/WorldMap";

export interface CoreRuntime { id: CoreId; name: string; object: Phaser.GameObjects.Arc; position: Point; delivered: boolean; }

export class CoreManager {
  private readonly cores = new Map<CoreId, CoreRuntime>();
  add(spec: CoreSpec, object: Phaser.GameObjects.Arc): void { this.cores.set(spec.id, { id: spec.id, name: spec.name, object, position: { ...spec.position }, delivered: false }); }
  reset(): void { this.cores.clear(); }
  get(id: CoreId): CoreRuntime | undefined { return this.cores.get(id); }
  values(): CoreRuntime[] { return [...this.cores.values()]; }
  nearestAvailable(position: Point, radius: number): CoreRuntime | null {
    return this.values().filter((core) => !core.delivered && core.object.visible && distance(position, core.position) <= radius).sort((a, b) => distance(position, a.position) - distance(position, b.position))[0] ?? null;
  }
  carry(id: CoreId): void { const core = this.cores.get(id); if (core) { core.object.setVisible(false); core.object.body && ((core.object.body as Phaser.Physics.Arcade.StaticBody).enable = false); } }
  drop(id: CoreId, position: Point): void { const core = this.cores.get(id); if (!core) return; core.position = { ...position }; core.object.setPosition(position.x, position.y).setVisible(true); if (core.object.body) { const body = core.object.body as Phaser.Physics.Arcade.StaticBody; body.enable = true; body.updateFromGameObject(); } }
  deliver(id: CoreId): void { const core = this.cores.get(id); if (!core) return; core.delivered = true; core.object.setVisible(false); if (core.object.body) (core.object.body as Phaser.Physics.Arcade.StaticBody).enable = false; }
}
