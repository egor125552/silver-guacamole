import type { GameCommand } from "../game/commands";
import type { KeyboardBindings } from "../game/types";
import { GameDiagnostics } from "../diagnostics/Diagnostics";
import { InputHub } from "./InputHub";

const aliases = (code: string, fallback: readonly string[]): readonly string[] => [code, ...fallback.filter((item) => item !== code)];

export class KeyboardInput {
  private readonly down = new Set<string>();
  private enabled = true;

  constructor(
    private readonly hub: InputHub,
    private readonly diagnostics: GameDiagnostics,
    private readonly bindings: () => KeyboardBindings,
  ) {}

  attach(): () => void {
    const keyDown = (event: KeyboardEvent) => {
      if (!this.enabled || this.isEditableTarget(event.target)) return;
      const bindings = this.bindings();
      const edgeCodes = [bindings.interact, bindings.special, bindings.status, bindings.instruction, bindings.pause, "Enter"];
      if (event.repeat && edgeCodes.includes(event.code)) return;
      const handled = this.handleEdge(event, bindings);
      if (this.continuousCodes(bindings).has(event.code)) {
        this.down.add(event.code);
        this.refreshAxes(bindings);
        this.diagnostics.setHeld(event.code, true);
      }
      if (handled || this.down.has(event.code)) event.preventDefault();
    };
    const keyUp = (event: KeyboardEvent) => {
      this.down.delete(event.code);
      this.refreshAxes(this.bindings());
      this.diagnostics.setHeld(event.code, false);
    };
    const clear = () => {
      this.down.clear();
      this.hub.clearHeld();
      this.diagnostics.resetHeld();
    };
    const visibility = () => { if (document.visibilityState !== "visible") clear(); };
    window.addEventListener("keydown", keyDown);
    window.addEventListener("keyup", keyUp);
    window.addEventListener("blur", clear);
    document.addEventListener("visibilitychange", visibility);
    return () => {
      window.removeEventListener("keydown", keyDown);
      window.removeEventListener("keyup", keyUp);
      window.removeEventListener("blur", clear);
      document.removeEventListener("visibilitychange", visibility);
      clear();
    };
  }

  setEnabled(enabled: boolean): void {
    this.enabled = enabled;
    if (!enabled) {
      this.down.clear();
      this.hub.clearHeld();
      this.diagnostics.resetHeld();
    }
  }

  private continuousCodes(bindings: KeyboardBindings): Set<string> {
    return new Set([
      ...aliases(bindings.forward, ["ArrowUp"]),
      ...aliases(bindings.back, ["ArrowDown"]),
      ...aliases(bindings.left, ["ArrowLeft"]),
      ...aliases(bindings.right, ["ArrowRight"]),
      ...aliases(bindings.fast, ["ShiftLeft", "ShiftRight"]),
    ]);
  }

  private refreshAxes(bindings: KeyboardBindings): void {
    const hasAny = (codes: readonly string[]): boolean => codes.some((code) => this.down.has(code));
    const forward = hasAny(aliases(bindings.forward, ["ArrowUp"]));
    const back = hasAny(aliases(bindings.back, ["ArrowDown"]));
    const fast = hasAny(aliases(bindings.fast, ["ShiftLeft", "ShiftRight"]));
    this.hub.setContinuousMove((forward ? (fast ? 1.7 : 1) : 0) + (back ? -0.65 : 0));
    const left = hasAny(aliases(bindings.left, ["ArrowLeft"]));
    const right = hasAny(aliases(bindings.right, ["ArrowRight"]));
    this.hub.setContinuousTurn((right ? 1 : 0) + (left ? -1 : 0));
  }

  private handleEdge(event: KeyboardEvent, bindings: KeyboardBindings): boolean {
    let command: GameCommand | null = null;
    if (event.code === bindings.interact || event.code === "Enter") command = { type: "interact" };
    else if (event.code === bindings.special) command = { type: "special" };
    else if (event.code === bindings.instruction) command = { type: "instruction" };
    else if (event.code === bindings.status) command = { type: "status" };
    else if (event.code === bindings.pause && event.shiftKey) command = { type: "emergency-stop" };
    else if (event.code === bindings.pause) command = { type: "pause" };
    if (!command) return false;
    this.hub.emit(command);
    return true;
  }

  private isEditableTarget(target: EventTarget | null): boolean {
    return target instanceof HTMLInputElement || target instanceof HTMLSelectElement || target instanceof HTMLTextAreaElement || (target instanceof HTMLElement && target.isContentEditable);
  }
}
