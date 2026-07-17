import type { KeyboardBindings, PersistedSettings, SaveData } from "../game/types";

const KEY = "midnight-switchyard-save-v2";
const LEGACY_KEY = "midnight-switchyard-save-v1";

export const DEFAULT_KEYBOARD: KeyboardBindings = {
  forward: "KeyW",
  back: "KeyS",
  left: "KeyA",
  right: "KeyD",
  fast: "ShiftLeft",
  interact: "Space",
  special: "KeyQ",
  status: "KeyI",
  instruction: "KeyH",
  pause: "Escape",
};

export const DEFAULT_SETTINGS: PersistedSettings = {
  inputMode: "keyboard",
  verbosity: "normal",
  speech: false,
  masterVolume: 0.85,
  gestureSensitivity: 1,
  visualScale: 1,
  keyboard: { ...DEFAULT_KEYBOARD },
};

const oneOf = <T extends string>(value: unknown, choices: readonly T[], fallback: T): T =>
  typeof value === "string" && choices.includes(value as T) ? value as T : fallback;
const finite = (value: unknown, fallback: number, min: number, max: number): number =>
  typeof value === "number" && Number.isFinite(value) ? Math.min(max, Math.max(min, value)) : fallback;
const keyCode = (value: unknown, fallback: string): string =>
  typeof value === "string" && /^[A-Za-z0-9]+$/.test(value) && value.length <= 24 ? value : fallback;

function normalizeKeyboard(value: unknown): KeyboardBindings {
  const source = value && typeof value === "object" ? value as Partial<KeyboardBindings> : {};
  return {
    forward: keyCode(source.forward, DEFAULT_KEYBOARD.forward),
    back: keyCode(source.back, DEFAULT_KEYBOARD.back),
    left: keyCode(source.left, DEFAULT_KEYBOARD.left),
    right: keyCode(source.right, DEFAULT_KEYBOARD.right),
    fast: keyCode(source.fast, DEFAULT_KEYBOARD.fast),
    interact: keyCode(source.interact, DEFAULT_KEYBOARD.interact),
    special: keyCode(source.special, DEFAULT_KEYBOARD.special),
    status: keyCode(source.status, DEFAULT_KEYBOARD.status),
    instruction: keyCode(source.instruction, DEFAULT_KEYBOARD.instruction),
    pause: keyCode(source.pause, DEFAULT_KEYBOARD.pause),
  };
}

function normalizeSettings(value: unknown): PersistedSettings {
  const source = value && typeof value === "object" ? value as Partial<PersistedSettings> : {};
  return {
    inputMode: oneOf(source.inputMode, ["voiceover", "gestures", "keyboard"] as const, DEFAULT_SETTINGS.inputMode),
    verbosity: oneOf(source.verbosity, ["minimal", "normal", "detailed"] as const, DEFAULT_SETTINGS.verbosity),
    speech: typeof source.speech === "boolean" ? source.speech : DEFAULT_SETTINGS.speech,
    masterVolume: finite(source.masterVolume, DEFAULT_SETTINGS.masterVolume, 0.2, 1),
    gestureSensitivity: finite(source.gestureSensitivity, DEFAULT_SETTINGS.gestureSensitivity, 0.6, 1.6),
    visualScale: finite(source.visualScale, DEFAULT_SETTINGS.visualScale, 0.8, 1.4),
    keyboard: normalizeKeyboard(source.keyboard),
  };
}

export class SaveStore {
  load(): SaveData {
    try {
      const raw = localStorage.getItem(KEY) ?? localStorage.getItem(LEGACY_KEY);
      if (!raw) return this.fresh();
      const parsed = JSON.parse(raw) as { version?: number; bestScore?: unknown; completedRuns?: unknown; settings?: unknown };
      if ((parsed.version !== 1 && parsed.version !== 2) || typeof parsed.bestScore !== "number" || typeof parsed.completedRuns !== "number") return this.fresh();
      const result: SaveData = {
        version: 2,
        bestScore: Math.max(0, Math.trunc(Number.isFinite(parsed.bestScore) ? parsed.bestScore : 0)),
        completedRuns: Math.max(0, Math.trunc(Number.isFinite(parsed.completedRuns) ? parsed.completedRuns : 0)),
        settings: normalizeSettings(parsed.settings),
      };
      if (parsed.version === 1) this.save(result);
      return result;
    } catch {
      return this.fresh();
    }
  }

  save(data: SaveData): void { localStorage.setItem(KEY, JSON.stringify({ ...data, version: 2 })); }

  recordWin(score: number): SaveData {
    const data = this.load();
    const safeScore = Number.isFinite(score) ? Math.max(0, Math.trunc(score)) : 0;
    const next: SaveData = { ...data, version: 2, bestScore: Math.max(data.bestScore, safeScore), completedRuns: data.completedRuns + 1 };
    this.save(next);
    return next;
  }

  saveSettings(settings: PersistedSettings): void {
    const data = this.load();
    this.save({ ...data, version: 2, settings: normalizeSettings(settings) });
  }

  clear(): SaveData {
    localStorage.removeItem(KEY);
    localStorage.removeItem(LEGACY_KEY);
    return this.fresh();
  }

  private fresh(): SaveData {
    return { version: 2, bestScore: 0, completedRuns: 0, settings: { ...DEFAULT_SETTINGS, keyboard: { ...DEFAULT_KEYBOARD } } };
  }
}
