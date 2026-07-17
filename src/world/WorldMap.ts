import type {
  DoorSpec,
  GridCell,
  Point,
  Rect,
  WorldSpec,
  ZoneName,
} from "../game/types";

export const AAJA_COMMIT = "4ad11f77b15163699d9213bc461d8aefb37c12f7";
export const TILE_SIZE = 64;
export const GRID_WIDTH = 32;
export const GRID_HEIGHT = 24;

const key = (col: number, row: number): string => `${col},${row}`;
const blocked = new Set<string>();
const block = (col: number, row: number): void => { blocked.add(key(col, row)); };
const unblock = (col: number, row: number): void => { blocked.delete(key(col, row)); };

for (let col = 0; col < GRID_WIDTH; col += 1) { block(col, 0); block(col, GRID_HEIGHT - 1); }
for (let row = 0; row < GRID_HEIGHT; row += 1) { block(0, row); block(GRID_WIDTH - 1, row); }

for (let row = 1; row < GRID_HEIGHT - 1; row += 1) block(7, row);
for (const row of [4, 18]) unblock(7, row);
for (let row = 1; row < GRID_HEIGHT - 1; row += 1) block(17, row);
for (const row of [4, 11, 19]) unblock(17, row);
for (let col = 18; col < GRID_WIDTH - 1; col += 1) block(col, 8);
unblock(24, 8);
for (let col = 18; col < GRID_WIDTH - 1; col += 1) block(col, 16);
unblock(24, 16);
for (let col = 8; col < 17; col += 1) block(col, 16);
unblock(12, 16);

for (let col = 2; col <= 5; col += 1) block(col, 10);
unblock(4, 10);
for (let col = 10; col <= 12; col += 1) { block(col, 6); block(col, 7); }
for (let col = 13; col <= 15; col += 1) { block(col, 12); block(col, 13); }
for (let row = 2; row <= 5; row += 1) block(21, row);
unblock(21, 4);
for (let col = 20; col <= 27; col += 1) block(col, 12);
for (const col of [22, 26]) unblock(col, 12);
for (let row = 18; row <= 21; row += 1) block(27, row);
unblock(27, 20);
for (let col = 9; col <= 15; col += 1) block(col, 21);
unblock(12, 21);

export const cellCenter = (col: number, row: number): Point => ({ x: col * TILE_SIZE + TILE_SIZE / 2, y: row * TILE_SIZE + TILE_SIZE / 2 });
export const worldToCell = (point: Point): GridCell => ({
  col: Math.max(0, Math.min(GRID_WIDTH - 1, Math.floor(point.x / TILE_SIZE))),
  row: Math.max(0, Math.min(GRID_HEIGHT - 1, Math.floor(point.y / TILE_SIZE))),
});
export const cellKey = (cell: GridCell): string => key(cell.col, cell.row);
export const cellRect = (col: number, row: number): Rect => ({ x: col * TILE_SIZE, y: row * TILE_SIZE, width: TILE_SIZE, height: TILE_SIZE });

const walls = [...blocked].map((entry) => {
  const parts = entry.split(",");
  return cellRect(Number(parts[0] ?? 0), Number(parts[1] ?? 0));
});

const verticalDoor = (id: DoorSpec["id"], name: string, col: number, row: number, initiallyOpen: boolean, requiresSwitch?: DoorSpec["requiresSwitch"]): DoorSpec => ({
  id, name, position: cellCenter(col, row), orientation: "vertical", initiallyOpen, ...(requiresSwitch ? { requiresSwitch } : {}),
});
const horizontalDoor = (id: DoorSpec["id"], name: string, col: number, row: number, initiallyOpen: boolean): DoorSpec => ({
  id, name, position: cellCenter(col, row), orientation: "horizontal", initiallyOpen,
});

export const WORLD: WorldSpec = {
  width: GRID_WIDTH * TILE_SIZE,
  height: GRID_HEIGHT * TILE_SIZE,
  tileSize: TILE_SIZE,
  gridWidth: GRID_WIDTH,
  gridHeight: GRID_HEIGHT,
  start: cellCenter(2, 2),
  walls,
  blockedCells: blocked,
  barriers: [
    { id: "short-east", rect: { x: 17 * TILE_SIZE + 10, y: 10 * TILE_SIZE + 8, width: 44, height: 112 } },
    { id: "machine-shortcut", rect: { x: 20 * TILE_SIZE + 8, y: 16 * TILE_SIZE + 10, width: 112, height: 44 } },
  ],
  doors: [
    verticalDoor("yard-north", "северные ворота двора", 7, 4, false),
    verticalDoor("yard-south", "южные ворота двора", 7, 18, true),
    verticalDoor("shaft-access", "дверь лифтовой шахты", 17, 4, false, "north"),
    verticalDoor("cooling-access", "дверь охлаждающего отсека", 17, 11, false),
    verticalDoor("machine-access", "дверь машинного помещения", 17, 19, false, "south"),
    horizontalDoor("shaft-cooling", "технический шлюз шахты", 24, 8, false),
    horizontalDoor("cooling-machine", "нижний технический шлюз", 24, 16, false),
    horizontalDoor("corridor-gate", "решётка технического коридора", 12, 16, false),
  ],
  cores: [
    { id: "amber", name: "янтарное ядро", position: cellCenter(3, 4) },
    { id: "cobalt", name: "кобальтовое ядро", position: cellCenter(28, 3) },
    { id: "violet", name: "фиолетовое ядро", position: cellCenter(28, 12) },
    { id: "emerald", name: "изумрудное ядро", position: cellCenter(28, 20) },
  ],
  switches: [
    { id: "north", name: "северный силовой переключатель", position: cellCenter(14, 4) },
    { id: "south", name: "южный силовой переключатель", position: cellCenter(14, 19) },
  ],
  coolPads: [cellCenter(4, 12), cellCenter(12, 9), cellCenter(22, 12), cellCenter(12, 19), cellCenter(24, 20)],
  lockers: [
    { id: "yard-armour", position: cellCenter(3, 20), reward: "health" },
    { id: "cooling-bolts", position: cellCenter(29, 10), reward: "bolts" },
  ],
  repairs: [
    { id: "yard-repair", position: cellCenter(4, 21) },
    { id: "shaft-repair", position: cellCenter(20, 3) },
  ],
  bay: cellCenter(12, 12),
  exit: cellCenter(29, 2),
  droneSpecs: [
    { id: "yard-listener", kind: "listener", route: [cellCenter(2, 7), cellCenter(5, 7), cellCenter(5, 17), cellCenter(2, 17)], speed: 78, hearingRadius: 480, sightRadius: 210 },
    { id: "hangar-sentinel", kind: "sentinel", route: [cellCenter(9, 3), cellCenter(15, 3), cellCenter(15, 14), cellCenter(9, 14)], speed: 88, hearingRadius: 320, sightRadius: 390 },
    { id: "shaft-sentinel", kind: "sentinel", route: [cellCenter(19, 2), cellCenter(29, 2), cellCenter(29, 6), cellCenter(19, 6)], speed: 84, hearingRadius: 330, sightRadius: 420 },
    { id: "cooling-listener", kind: "listener", route: [cellCenter(19, 10), cellCenter(29, 10), cellCenter(29, 14), cellCenter(19, 14)], speed: 80, hearingRadius: 520, sightRadius: 240 },
    { id: "machine-sentinel", kind: "sentinel", route: [cellCenter(19, 18), cellCenter(29, 18), cellCenter(29, 21), cellCenter(19, 21)], speed: 92, hearingRadius: 360, sightRadius: 400 },
    { id: "lockdown-interceptor", kind: "interceptor", route: [cellCenter(18, 9), cellCenter(25, 9), cellCenter(25, 14), cellCenter(18, 14)], speed: 116, hearingRadius: 600, sightRadius: 480, activateOnLockdown: true },
  ],
  trolleyRouteA: [cellCenter(8, 9), cellCenter(16, 9), cellCenter(16, 14), cellCenter(8, 14)],
  trolleyRouteB: [cellCenter(8, 9), cellCenter(16, 9), cellCenter(18, 10), cellCenter(25, 10), cellCenter(25, 15), cellCenter(16, 15), cellCenter(8, 14)],
  zones: [
    { name: "yard", label: "открытый сортировочный двор", rect: { x: 64, y: 64, width: 384, height: 1408 }, room: "outdoors", surface: "gravel", ambience: "yard-ambience", absorption: 0.08 },
    { name: "hangar", label: "металлический ангар", rect: { x: 512, y: 64, width: 576, height: 960 }, room: "metal-room", surface: "metal", ambience: "hangar-ambience", absorption: 0.18 },
    { name: "corridor", label: "узкий технический коридор", rect: { x: 512, y: 1088, width: 576, height: 384 }, room: "metal-corridor", surface: "grating", ambience: "corridor-ambience", absorption: 0.34 },
    { name: "shaft", label: "лифтовая шахта", rect: { x: 1152, y: 64, width: 832, height: 448 }, room: "large-room", surface: "concrete", ambience: "shaft-ambience", absorption: 0.24 },
    { name: "cooling", label: "охлаждающий отсек", rect: { x: 1152, y: 576, width: 832, height: 448 }, room: "small-room", surface: "rubber", ambience: "cooling-ambience", absorption: 0.46 },
    { name: "machine", label: "машинное помещение", rect: { x: 1152, y: 1088, width: 832, height: 384 }, room: "basement", surface: "concrete", ambience: "machine-ambience", absorption: 0.52 },
  ],
};

export function distance(a: Point, b: Point): number { return Math.hypot(a.x - b.x, a.y - b.y); }
export function pointInRect(point: Point, rect: Rect): boolean {
  return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y && point.y <= rect.y + rect.height;
}
export function zoneAt(point: Point): ZoneName {
  return WORLD.zones.find((zone) => pointInRect(point, zone.rect))?.name ?? "hangar";
}
export function lineIntersectsRect(a: Point, b: Point, rect: Rect): boolean {
  const steps = Math.max(4, Math.ceil(distance(a, b) / 24));
  for (let index = 0; index <= steps; index += 1) {
    const t = index / steps;
    if (pointInRect({ x: a.x + (b.x - a.x) * t, y: a.y + (b.y - a.y) * t }, rect)) return true;
  }
  return false;
}
export function doorRect(door: DoorSpec): Rect {
  return door.orientation === "vertical"
    ? { x: door.position.x - 16, y: door.position.y - 29, width: 32, height: 58 }
    : { x: door.position.x - 29, y: door.position.y - 16, width: 58, height: 32 };
}
