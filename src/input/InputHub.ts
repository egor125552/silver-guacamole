import type { CommandListener, GameCommand } from "../game/commands";

export class InputHub {
  private readonly listeners = new Set<CommandListener>();
  private moveAmount = 0;
  private turnAmount = 0;
  private pulseUntil = 0;
  private pulseMove = 0;

  subscribe(listener: CommandListener): () => void { this.listeners.add(listener); return () => this.listeners.delete(listener); }
  emit(command: GameCommand): void {
    if (command.type === "move" && command.durationMs) {
      this.pulseMove = command.amount;
      this.pulseUntil = performance.now() + command.durationMs;
    }
    for (const listener of this.listeners) listener(command);
  }

  setContinuousMove(amount: number): void { this.moveAmount = amount; }
  setContinuousTurn(amount: number): void { this.turnAmount = amount; }
  currentMove(): number { return performance.now() < this.pulseUntil ? this.pulseMove : this.moveAmount; }
  currentTurn(): number { return this.turnAmount; }
  clearHeld(): void { this.moveAmount = 0; this.turnAmount = 0; this.pulseUntil = 0; this.pulseMove = 0; }
}
