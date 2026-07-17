import type { Point } from "../game/types";

export interface TestSnapshot {
  phase: string;
  player: Point;
  angle: number;
  health: number;
  bolts: number;
  carriedCore: string | null;
  delivered: string[];
  heat: number;
  switches: { north: boolean; south: boolean };
  doors: Record<string, boolean>;
  droneStates: Record<string, string>;
  noiseEvents: number;
  activeSources: number;
  heldCommands: number;
  errors: number;
  elapsedMs: number;
  barriers: number;
}

export type TestTargets = Record<string, Point | Point[] | Record<string, Point>>;

export interface TestBridgeApi {
  snapshot: () => TestSnapshot;
  planPath: (x: number, y: number) => Point[];
  moveTo: (x: number, y: number) => void;
  targets: () => TestTargets;
}

declare global {
  interface Window {
    __SWITCHYARD_TEST__?: TestBridgeApi;
  }
}

export function installTestBridge(enabled: boolean, api: TestBridgeApi): () => void {
  if (enabled) window.__SWITCHYARD_TEST__ = api;
  else delete window.__SWITCHYARD_TEST__;
  return () => { delete window.__SWITCHYARD_TEST__; };
}
