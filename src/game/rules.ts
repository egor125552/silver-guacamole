import type { CoreId, GamePhase, Point, SwitchId } from "./types";

const CORE_HEAT_PER_MS = 0.00155;
const CORE_COOL_PER_MS = 0.0125;
const UNATTENDED_CORE_COOL_PER_MS = 0.0065;

export interface RulesSnapshot {
  phase: GamePhase;
  health: number;
  maxHealth: number;
  bolts: number;
  maxBolts: number;
  carriedCore: CoreId | null;
  delivered: ReadonlySet<CoreId>;
  coreHeat: number;
  switchNorth: boolean;
  switchSouth: boolean;
  openedLockers: ReadonlySet<string>;
  usedRepairs: ReadonlySet<string>;
  elapsedMs: number;
  damageTaken: number;
  lockdownRemainingMs: number;
  requiredCores: number;
}

export type RulesEvent =
  | { type: "announce"; text: string; priority?: "normal" | "danger" }
  | { type: "phase"; phase: GamePhase }
  | { type: "core-dropped"; core: CoreId; position: Point }
  | { type: "core-delivered"; core: CoreId; delivered: number; required: number }
  | { type: "damage"; amount: number; source: string }
  | { type: "repair"; id: string }
  | { type: "switch"; id: SwitchId; value: boolean }
  | { type: "bolt" }
  | { type: "lockdown" }
  | { type: "win"; score: number }
  | { type: "lose"; reason: string };

export class GameRules {
  phase: GamePhase = "ready";
  health = 3;
  maxHealth = 3;
  bolts = 3;
  maxBolts = 3;
  carriedCore: CoreId | null = null;
  readonly delivered = new Set<CoreId>();
  coreHeat = 0;
  switchNorth = false;
  switchSouth = false;
  readonly openedLockers = new Set<string>();
  readonly usedRepairs = new Set<string>();
  elapsedMs = 0;
  damageTaken = 0;
  lockdownRemainingMs = 0;
  private damageCooldownMs = 0;
  private finalScore = 0;
  readonly events: RulesEvent[] = [];

  constructor(readonly requiredCores = 4) {}

  reset(): void {
    this.phase = "ready";
    this.health = 3;
    this.maxHealth = 3;
    this.bolts = 3;
    this.maxBolts = 3;
    this.carriedCore = null;
    this.delivered.clear();
    this.coreHeat = 0;
    this.switchNorth = false;
    this.switchSouth = false;
    this.openedLockers.clear();
    this.usedRepairs.clear();
    this.elapsedMs = 0;
    this.damageTaken = 0;
    this.lockdownRemainingMs = 0;
    this.damageCooldownMs = 0;
    this.finalScore = 0;
    this.events.length = 0;
  }

  start(): void {
    if (this.phase !== "ready") return;
    this.phase = "running";
    this.events.push(
      { type: "phase", phase: this.phase },
      { type: "announce", text: `Смена началась. Найди и доставь ${this.requiredCores} перегревающихся энергоядра в центральный подъёмник.` },
    );
  }

  update(deltaMs: number, onCoolingPad: boolean, playerPosition: Point): void {
    if (this.phase !== "running" && this.phase !== "lockdown") return;
    this.elapsedMs += deltaMs;
    this.damageCooldownMs = Math.max(0, this.damageCooldownMs - deltaMs);
    if (this.carriedCore) {
      const heatDelta = onCoolingPad ? -CORE_COOL_PER_MS : CORE_HEAT_PER_MS;
      this.coreHeat = Math.max(0, Math.min(100, this.coreHeat + deltaMs * heatDelta));
      if (this.coreHeat >= 100) {
        const core = this.carriedCore;
        this.carriedCore = null;
        this.coreHeat = 58;
        this.events.push(
          { type: "announce", text: "Ядро перегрелось, вырвалось из захвата и упало рядом.", priority: "danger" },
          { type: "core-dropped", core, position: playerPosition },
        );
        this.damage(1, "выброс перегретого ядра");
      }
    } else {
      this.coreHeat = Math.max(0, this.coreHeat - deltaMs * UNATTENDED_CORE_COOL_PER_MS);
    }
    if (this.phase === "lockdown") {
      this.lockdownRemainingMs = Math.max(0, this.lockdownRemainingMs - deltaMs);
      if (this.lockdownRemainingMs === 0) this.lose("Аварийный шлюз закрылся до запуска лифта.");
    }
  }

  pickCore(core: CoreId): boolean {
    if (this.carriedCore || this.delivered.has(core) || this.phase !== "running") return false;
    this.carriedCore = core;
    this.coreHeat = Math.max(this.coreHeat, 7);
    this.events.push({ type: "announce", text: "Ядро поднято. Оно нагревается. Охлаждающие площадки дают время, но заставляют выбирать более длинный маршрут." });
    return true;
  }

  dropCore(position: Point): CoreId | null {
    if (!this.carriedCore) return null;
    const core = this.carriedCore;
    this.carriedCore = null;
    this.events.push({ type: "core-dropped", core, position }, { type: "announce", text: "Ядро поставлено на пол и начинает остывать." });
    return core;
  }

  deliverCore(): CoreId | null {
    const core = this.carriedCore;
    if (!core || this.delivered.has(core)) return null;
    this.carriedCore = null;
    this.delivered.add(core);
    this.coreHeat = 0;
    this.events.push(
      { type: "core-delivered", core, delivered: this.delivered.size, required: this.requiredCores },
      { type: "announce", text: `Ядро принято. Доставлено ${this.delivered.size} из ${this.requiredCores}. Патрули усиливаются.` },
    );
    if (this.delivered.size === this.requiredCores) this.beginLockdown();
    return core;
  }

  toggleSwitch(id: SwitchId): boolean {
    const next = id === "north" ? !this.switchNorth : !this.switchSouth;
    if (id === "north") this.switchNorth = next;
    else this.switchSouth = next;
    this.events.push(
      { type: "switch", id, value: next },
      { type: "announce", text: `${id === "north" ? "Северный" : "Южный"} силовой переключатель ${next ? "включён" : "выключен"}.` },
    );
    return next;
  }

  useBolt(): boolean {
    if (this.bolts <= 0 || (this.phase !== "running" && this.phase !== "lockdown")) return false;
    this.bolts -= 1;
    this.events.push({ type: "bolt" });
    return true;
  }

  openLocker(id: string, reward: "health" | "bolts"): boolean {
    if (this.openedLockers.has(id)) return false;
    this.openedLockers.add(id);
    if (reward === "health") {
      this.maxHealth = Math.min(5, this.maxHealth + 1);
      this.health = this.maxHealth;
      this.events.push({ type: "announce", text: "Найдена усиленная защита. Прочность восстановлена и увеличена." });
    } else {
      this.maxBolts = Math.min(7, this.maxBolts + 3);
      this.bolts = this.maxBolts;
      this.events.push({ type: "announce", text: "Найден запас стальных болтов для шумовых приманок." });
    }
    return true;
  }

  repair(id: string): boolean {
    if (this.usedRepairs.has(id) || this.health >= this.maxHealth) return false;
    this.usedRepairs.add(id);
    this.health = this.maxHealth;
    this.events.push({ type: "repair", id }, { type: "announce", text: "Ремонтный пост восстановил защиту. Повторно он не сработает." });
    return true;
  }

  damage(amount: number, source: string): void {
    if (this.damageCooldownMs > 0 || this.phase === "lost" || this.phase === "won") return;
    this.damageCooldownMs = 1050;
    this.health = Math.max(0, this.health - amount);
    this.damageTaken += amount;
    this.events.push(
      { type: "damage", amount, source },
      { type: "announce", text: `Удар: ${source}. Прочность ${this.health} из ${this.maxHealth}.`, priority: "danger" },
    );
    if (this.health <= 0) this.lose(`Защита не выдержала: ${source}.`);
  }

  canExit(): boolean { return this.phase === "lockdown" && this.switchNorth && this.switchSouth; }

  win(): number {
    if (!this.canExit()) return 0;
    this.phase = "won";
    const timePenalty = Math.floor(this.elapsedMs / 1000);
    this.finalScore = Math.max(100, 7000 - timePenalty - this.damageTaken * 320 + this.openedLockers.size * 180 + this.bolts * 70 + this.usedRepairs.size * 60);
    this.events.push({ type: "phase", phase: this.phase }, { type: "win", score: this.finalScore });
    return this.finalScore;
  }

  lose(reason: string): void {
    if (this.phase === "lost" || this.phase === "won") return;
    this.phase = "lost";
    this.events.push({ type: "phase", phase: this.phase }, { type: "lose", reason });
  }

  pause(value: boolean): void {
    if (value && (this.phase === "running" || this.phase === "lockdown")) this.phase = "paused";
    else if (!value && this.phase === "paused") this.phase = this.delivered.size === this.requiredCores ? "lockdown" : "running";
    this.events.push({ type: "phase", phase: this.phase });
  }

  snapshot(): RulesSnapshot {
    return {
      phase: this.phase,
      health: this.health,
      maxHealth: this.maxHealth,
      bolts: this.bolts,
      maxBolts: this.maxBolts,
      carriedCore: this.carriedCore,
      delivered: new Set(this.delivered),
      coreHeat: this.coreHeat,
      switchNorth: this.switchNorth,
      switchSouth: this.switchSouth,
      openedLockers: new Set(this.openedLockers),
      usedRepairs: new Set(this.usedRepairs),
      elapsedMs: this.elapsedMs,
      damageTaken: this.damageTaken,
      lockdownRemainingMs: this.lockdownRemainingMs,
      requiredCores: this.requiredCores,
    };
  }

  private beginLockdown(): void {
    this.phase = "lockdown";
    this.switchNorth = false;
    this.switchSouth = false;
    this.lockdownRemainingMs = 180_000;
    this.events.push(
      { type: "phase", phase: this.phase },
      { type: "lockdown" },
      { type: "announce", text: "Финальное испытание: питание дверей сброшено, активирован перехватчик. Включи оба силовых переключателя и доберись до диспетчерской за три минуты.", priority: "danger" },
    );
  }
}
