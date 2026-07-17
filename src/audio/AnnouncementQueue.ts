import type { PersistedSettings } from "../game/types";
import { AajaAudioAdapter } from "./AajaAudioAdapter";

interface Item { text: string; priority: "normal" | "danger"; }

export class AnnouncementQueue {
  private readonly queue: Item[] = [];
  private speaking = false;
  private currentUtterance: SpeechSynthesisUtterance | null = null;

  constructor(
    private readonly audio: AajaAudioAdapter,
    private readonly settings: () => PersistedSettings,
    private readonly publishStatus: (text: string, assertive: boolean) => void,
  ) {}

  announce(text: string, priority: "normal" | "danger" = "normal"): void {
    this.publishStatus(text, priority === "danger");
    if (!this.settings().speech || !("speechSynthesis" in window)) return;
    if (priority === "danger") {
      this.queue.length = 0;
      if (this.currentUtterance) window.speechSynthesis.cancel();
      this.speaking = false;
    }
    this.queue.push({ text, priority });
    this.pump();
  }

  stop(): void { this.queue.length = 0; window.speechSynthesis?.cancel(); this.speaking = false; this.currentUtterance = null; this.audio.setSpeechDucking(false); }

  private pump(): void {
    if (this.speaking || this.queue.length === 0) return;
    const item = this.queue.shift(); if (!item) return;
    this.speaking = true; this.audio.setSpeechDucking(true);
    const utterance = new SpeechSynthesisUtterance(item.text);
    utterance.lang = "ru-RU"; utterance.rate = 1.08; utterance.volume = 0.95;
    const done = () => { this.speaking = false; this.currentUtterance = null; this.audio.setSpeechDucking(false); this.pump(); };
    utterance.onend = done; utterance.onerror = done; this.currentUtterance = utterance; window.speechSynthesis.speak(utterance);
  }
}
