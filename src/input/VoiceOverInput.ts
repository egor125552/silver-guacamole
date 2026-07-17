import { InputHub } from "./InputHub";

export class VoiceOverInput {
  constructor(private readonly hub: InputHub) {}
  attach(root: HTMLElement): () => void {
    const testMode = new URLSearchParams(window.location.search).get("testMode") === "1";
    const duration = (normalMs: number): number => testMode ? Math.round(normalMs / 2) : normalMs;
    const onClick = (event: Event) => {
      const button = (event.target as HTMLElement).closest<HTMLButtonElement>("button[data-command]");
      if (!button) return;
      const command = button.dataset.command;
      if (command === "forward") this.hub.emit({ type: "move", amount: 1, durationMs: duration(510) });
      else if (command === "fast") this.hub.emit({ type: "move", amount: 1.6, durationMs: duration(620) });
      else if (command === "back") this.hub.emit({ type: "move", amount: -0.6, durationMs: duration(500) });
      else if (command === "left") this.hub.emit({ type: "turn", amount: -Math.PI / 2 });
      else if (command === "right") this.hub.emit({ type: "turn", amount: Math.PI / 2 });
      else if (command === "interact") this.hub.emit({ type: "interact" });
      else if (command === "special") this.hub.emit({ type: "special" });
      else if (command === "status") this.hub.emit({ type: "status" });
      else if (command === "instruction") this.hub.emit({ type: "instruction" });
      else if (command === "pause") this.hub.emit({ type: "pause" });
      else if (command === "stop") this.hub.emit({ type: "emergency-stop" });
    };
    root.addEventListener("click", onClick);
    return () => root.removeEventListener("click", onClick);
  }
}
