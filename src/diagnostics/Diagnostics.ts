export interface DiagnosticEntry { time: number; level: "info" | "warning" | "error"; code: string; message: string; }

export class GameDiagnostics {
  private readonly entries: DiagnosticEntry[] = [];
  private readonly heldInputs = new Set<string>();

  log(level: DiagnosticEntry["level"], code: string, message: string): void {
    const entry = { time: performance.now(), level, code, message };
    this.entries.push(entry);
    if (level === "error") console.error(`[${code}] ${message}`);
    else if (level === "warning") console.warn(`[${code}] ${message}`);
  }

  setHeld(name: string, active: boolean): void { if (active) this.heldInputs.add(name); else this.heldInputs.delete(name); }
  resetHeld(): void { this.heldInputs.clear(); }
  snapshot(): ReadonlyArray<DiagnosticEntry> { return this.entries.slice(); }
  heldCount(): number { return this.heldInputs.size; }
  errorCount(): number { return this.entries.filter((entry) => entry.level === "error").length; }
}
