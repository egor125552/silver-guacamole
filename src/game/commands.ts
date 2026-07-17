export type GameCommand =
  | { type: "move"; amount: number; durationMs?: number }
  | { type: "turn"; amount: number }
  | { type: "interact" }
  | { type: "special" }
  | { type: "status" }
  | { type: "instruction" }
  | { type: "pause" }
  | { type: "emergency-stop" };

export type CommandListener = (command: GameCommand) => void;
