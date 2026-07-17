export type Point = Readonly<{ x: number; y: number }>;
export type Rect = Readonly<{ x: number; y: number; width: number; height: number }>;
export type GridCell = Readonly<{ col: number; row: number }>;
export type ZoneName = "yard" | "hangar" | "corridor" | "cooling" | "shaft" | "machine";
export type SurfaceName = "gravel" | "metal" | "concrete" | "grating" | "rubber";
export type CoreId = "amber" | "cobalt" | "violet" | "emerald";
export type SwitchId = "north" | "south";
export type DoorId =
  | "yard-north"
  | "yard-south"
  | "shaft-access"
  | "cooling-access"
  | "machine-access"
  | "shaft-cooling"
  | "cooling-machine"
  | "corridor-gate";
export type GamePhase = "ready" | "running" | "lockdown" | "won" | "lost" | "paused";
export type DroneKind = "sentinel" | "listener" | "interceptor";
export type DroneState = "dormant" | "patrol" | "investigate" | "chase" | "search" | "return" | "stunned";

export interface CoreSpec { id: CoreId; name: string; position: Point; }
export interface SwitchSpec { id: SwitchId; name: string; position: Point; }
export interface LockerSpec { id: string; position: Point; reward: "health" | "bolts"; }
export interface RepairSpec { id: string; position: Point; }
export interface DoorSpec {
  id: DoorId;
  name: string;
  position: Point;
  orientation: "vertical" | "horizontal";
  initiallyOpen: boolean;
  requiresSwitch?: SwitchId;
}
export interface BarrierSpec { id: string; rect: Rect; }
export interface DroneSpec {
  id: string;
  kind: DroneKind;
  route: readonly Point[];
  speed: number;
  hearingRadius: number;
  sightRadius: number;
  activateOnLockdown?: boolean;
}
export interface ZoneSpec {
  name: ZoneName;
  label: string;
  rect: Rect;
  room: "outdoors" | "metal-room" | "metal-corridor" | "small-room" | "basement" | "large-room";
  surface: SurfaceName;
  ambience: string;
  absorption: number;
}

export interface WorldSpec {
  width: number;
  height: number;
  tileSize: number;
  gridWidth: number;
  gridHeight: number;
  start: Point;
  walls: readonly Rect[];
  blockedCells: ReadonlySet<string>;
  barriers: readonly BarrierSpec[];
  doors: readonly DoorSpec[];
  cores: readonly CoreSpec[];
  switches: readonly SwitchSpec[];
  coolPads: readonly Point[];
  lockers: readonly LockerSpec[];
  repairs: readonly RepairSpec[];
  bay: Point;
  exit: Point;
  droneSpecs: readonly DroneSpec[];
  trolleyRouteA: readonly Point[];
  trolleyRouteB: readonly Point[];
  zones: readonly ZoneSpec[];
}

export interface KeyboardBindings {
  forward: string;
  back: string;
  left: string;
  right: string;
  fast: string;
  interact: string;
  special: string;
  status: string;
  instruction: string;
  pause: string;
}

export interface PersistedSettings {
  inputMode: "voiceover" | "gestures" | "keyboard";
  verbosity: "minimal" | "normal" | "detailed";
  speech: boolean;
  masterVolume: number;
  gestureSensitivity: number;
  visualScale: number;
  keyboard: KeyboardBindings;
}

export interface SaveData {
  version: 2;
  bestScore: number;
  completedRuns: number;
  settings: PersistedSettings;
}
