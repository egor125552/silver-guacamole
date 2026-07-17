import Phaser from "phaser";
import { AnnouncementQueue } from "./audio/AnnouncementQueue";
import { AajaAudioAdapter } from "./audio/AajaAudioAdapter";
import { AudioScene } from "./audio/AudioScene";
import { GameDiagnostics } from "./diagnostics/Diagnostics";
import type { PersistedSettings } from "./game/types";
import { GestureInput } from "./input/GestureInput";
import { InputHub } from "./input/InputHub";
import { KeyboardInput } from "./input/KeyboardInput";
import { VoiceOverInput } from "./input/VoiceOverInput";
import { SaveStore } from "./save/SaveStore";
import { GameScene } from "./scenes/GameScene";
import { AppUI } from "./ui/AppUI";

export class GameBootstrap {
  private readonly saveStore = new SaveStore();
  private saveData = this.saveStore.load();
  private settings: PersistedSettings = this.saveData.settings;
  private readonly ui: AppUI;
  private readonly input = new InputHub();
  private readonly diagnostics = new GameDiagnostics();
  private readonly keyboard = new KeyboardInput(this.input, this.diagnostics, () => this.settings.keyboard);
  private readonly voiceover = new VoiceOverInput(this.input);
  private gesture: GestureInput;
  private readonly audio: AajaAudioAdapter;
  private readonly audioScene: AudioScene;
  private readonly announcements: AnnouncementQueue;
  private game: Phaser.Game | null = null;
  private detachKeyboard: (() => void) | null = null;
  private detachVoiceover: (() => void) | null = null;
  private detachGesture: (() => void) | null = null;

  constructor(container: HTMLElement) {
    this.ui = new AppUI(container, this.saveData);
    this.audio = new AajaAudioAdapter(this.diagnostics, () => this.settings.masterVolume);
    this.audioScene = new AudioScene(this.audio);
    this.announcements = new AnnouncementQueue(this.audio, () => this.settings, (text, assertive) => this.ui.announce(text, assertive));
    this.gesture = new GestureInput(this.input, this.diagnostics, () => this.settings.gestureSensitivity);
  }

  attach(): void {
    window.addEventListener("error", (event) => this.diagnostics.log("error", "window.error", event.error instanceof Error ? event.error.message : event.message));
    window.addEventListener("unhandledrejection", (event) => this.diagnostics.log("error", "window.unhandledrejection", event.reason instanceof Error ? event.reason.message : String(event.reason)));
    this.detachKeyboard = this.keyboard.attach();
    this.detachVoiceover = this.voiceover.attach(this.ui.root);
    this.ui.onStart(({ settings }) => void this.start(settings));
    this.ui.onRestart(() => void this.restart());
    this.ui.onReset(() => this.resetSave());
    this.input.subscribe((command) => {
      if (command.type !== "emergency-stop" && this.audio.isStarted()) void this.audio.resumeAfterStop();
    });
  }

  private async start(settings: PersistedSettings): Promise<void> {
    this.settings = settings;
    this.saveStore.saveSettings(settings);
    this.saveData = this.saveStore.load();
    try {
      await this.audio.start();
    } catch {
      this.ui.announce("Не удалось запустить Aaja Audio Engine. Проверь поддержку Web Audio и WASM.", true);
      return;
    }
    this.configureInputMode();
    this.ui.showGame(settings);
    this.createGame();
  }

  private createGame(): void {
    this.game?.destroy(true);
    const scene = new GameScene({
      input: this.input,
      audio: this.audio,
      audioScene: this.audioScene,
      announcements: this.announcements,
      diagnostics: this.diagnostics,
      ui: this.ui,
      settings: () => this.settings,
      save: this.saveStore,
      onRestart: () => void this.restart(),
    });
    this.game = new Phaser.Game({
      type: Phaser.AUTO,
      parent: this.ui.canvasHost,
      width: 1024,
      height: 768,
      backgroundColor: "#101820",
      scene: [scene],
      physics: { default: "arcade", arcade: { gravity: { x: 0, y: 0 }, debug: false } },
      render: { antialias: true, pixelArt: false, roundPixels: false },
      scale: { mode: Phaser.Scale.FIT, autoCenter: Phaser.Scale.CENTER_BOTH },
      input: { activePointers: 1 },
      audio: { disableWebAudio: true, noAudio: true },
      banner: false,
    });
  }

  private configureInputMode(): void {
    this.keyboard.setEnabled(this.settings.inputMode === "keyboard");
    this.detachGesture?.(); this.detachGesture = null;
    if (this.settings.inputMode === "gestures") this.detachGesture = this.gesture.attach(this.ui.gestureSurface);
  }

  private async restart(): Promise<void> {
    this.announcements.stop(); this.input.clearHeld(); await this.audio.resumeAfterStop();
    this.game?.scene.stop("GameScene"); this.game?.scene.start("GameScene");
    this.ui.hidePause(); this.ui.announce("Новая смена началась.", false);
  }

  private resetSave(): void {
    this.saveData = this.saveStore.clear(); this.settings = this.saveData.settings; this.ui.showSetup(this.saveData);
  }
}
