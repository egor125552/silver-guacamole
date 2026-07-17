import type { SurfaceName } from "../game/types";

export const surfaceNoise: Record<SurfaceName, number> = {
  gravel: 350,
  metal: 430,
  concrete: 310,
  grating: 480,
  rubber: 210,
};

export const coreColours: Record<string, number> = {
  amber: 0xf1a43c,
  cobalt: 0x4aa7ff,
  violet: 0xa879ff,
  emerald: 0x45d88b,
};

export const droneColours: Record<string, number> = {
  sentinel: 0xe56b6f,
  listener: 0xffb347,
  interceptor: 0xff4057,
};
