import type { DoorId, DoorSpec, Rect, SwitchId } from "../game/types";
import { cellKey, doorRect, worldToCell } from "./WorldMap";

export class DoorSystem {
  private readonly open = new Map<DoorId, boolean>();
  constructor(readonly specs: readonly DoorSpec[]) { for (const door of specs) this.open.set(door.id, door.initiallyOpen); }

  reset(): void { for (const door of this.specs) this.open.set(door.id, door.initiallyOpen); }
  isOpen(id: DoorId): boolean { return this.open.get(id) ?? false; }
  setOpen(id: DoorId, value: boolean): boolean { const changed = this.isOpen(id) !== value; this.open.set(id, value); return changed; }
  toggle(id: DoorId): boolean { const value = !this.isOpen(id); this.open.set(id, value); return value; }
  spec(id: DoorId): DoorSpec { const door = this.specs.find((item) => item.id === id); if (!door) throw new Error(`Unknown door ${id}`); return door; }
  syncSwitch(id: SwitchId, value: boolean): DoorId[] {
    const changed: DoorId[] = [];
    for (const door of this.specs) if (door.requiresSwitch === id && this.setOpen(door.id, value)) changed.push(door.id);
    return changed;
  }
  closePoweredDoors(): DoorId[] {
    const changed: DoorId[] = [];
    for (const door of this.specs) if (door.requiresSwitch && this.setOpen(door.id, false)) changed.push(door.id);
    return changed;
  }
  blockingRects(): Rect[] { return this.specs.filter((door) => !this.isOpen(door.id)).map(doorRect); }
  blockingCells(): Set<string> {
    return new Set(this.specs.filter((door) => !this.isOpen(door.id)).map((door) => cellKey(worldToCell(door.position))));
  }
}
