import { GameDiagnostics } from "../diagnostics/Diagnostics";
import { InputHub } from "./InputHub";

export class GestureInput {
  private pointerId: number | null = null;
  private startX = 0;
  private startY = 0;
  private startTime = 0;
  private tapCount = 0;
  private tapTimer: number | null = null;

  constructor(private readonly hub: InputHub, private readonly diagnostics: GameDiagnostics, private readonly sensitivity: () => number) {}

  attach(surface: HTMLElement): () => void {
    const down = (event: PointerEvent) => {
      if (this.pointerId !== null) return;
      this.pointerId = event.pointerId;
      this.startX = event.clientX;
      this.startY = event.clientY;
      this.startTime = performance.now();
      surface.setPointerCapture(event.pointerId);
      this.diagnostics.setHeld("gesture", true);
      event.preventDefault();
    };
    const up = (event: PointerEvent) => {
      if (event.pointerId !== this.pointerId) return;
      const dx = event.clientX - this.startX;
      const dy = event.clientY - this.startY;
      const elapsed = performance.now() - this.startTime;
      this.finish(surface, event.pointerId);
      const threshold = 42 / Math.max(0.6, this.sensitivity());
      const magnitude = Math.hypot(dx, dy);
      if (magnitude < threshold) {
        if (elapsed >= 620) this.hub.emit({ type: "status" });
        else this.queueTap();
      } else if (elapsed >= 720 && dy > threshold * 2 && Math.abs(dy) > Math.abs(dx) * 1.4) {
        this.hub.emit({ type: "emergency-stop" });
      } else if (Math.abs(dx) > Math.abs(dy)) {
        this.hub.emit({ type: "turn", amount: dx > 0 ? Math.PI / 2 : -Math.PI / 2 });
      } else {
        this.hub.emit({ type: "move", amount: dy < 0 ? 1 : -0.6, durationMs: 520 });
      }
      event.preventDefault();
    };
    const cancel = (event: PointerEvent) => { if (event.pointerId === this.pointerId) this.finish(surface, event.pointerId); };
    const clear = () => {
      if (this.pointerId !== null) this.finish(surface, this.pointerId);
      if (this.tapTimer !== null) window.clearTimeout(this.tapTimer);
      this.tapTimer = null;
      this.tapCount = 0;
      this.hub.clearHeld();
    };
    const visibility = () => { if (document.visibilityState !== "visible") clear(); };
    surface.addEventListener("pointerdown", down);
    surface.addEventListener("pointerup", up);
    surface.addEventListener("pointercancel", cancel);
    surface.addEventListener("lostpointercapture", cancel);
    window.addEventListener("blur", clear);
    document.addEventListener("visibilitychange", visibility);
    return () => {
      surface.removeEventListener("pointerdown", down);
      surface.removeEventListener("pointerup", up);
      surface.removeEventListener("pointercancel", cancel);
      surface.removeEventListener("lostpointercapture", cancel);
      window.removeEventListener("blur", clear);
      document.removeEventListener("visibilitychange", visibility);
      clear();
    };
  }

  private queueTap(): void {
    this.tapCount += 1;
    if (this.tapTimer !== null) window.clearTimeout(this.tapTimer);
    this.tapTimer = window.setTimeout(() => {
      const count = this.tapCount;
      this.tapCount = 0;
      this.tapTimer = null;
      if (count === 1) this.hub.emit({ type: "interact" });
      else if (count === 2) this.hub.emit({ type: "special" });
      else if (count === 3) this.hub.emit({ type: "pause" });
      else this.hub.emit({ type: "instruction" });
    }, 330);
  }

  private finish(surface: HTMLElement, id: number): void {
    if (surface.hasPointerCapture(id)) surface.releasePointerCapture(id);
    this.pointerId = null;
    this.hub.clearHeld();
    this.diagnostics.setHeld("gesture", false);
  }
}
