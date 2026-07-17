import type { KeyboardBindings, PersistedSettings, SaveData } from "../game/types";

export interface StartOptions { settings: PersistedSettings; }

const bindingLabels: ReadonlyArray<[keyof KeyboardBindings, string]> = [
  ["forward", "Вперёд"], ["back", "Назад"], ["left", "Поворот влево"], ["right", "Поворот вправо"],
  ["fast", "Быстро"], ["interact", "Действие"], ["special", "Бросить болт"], ["status", "Статус"],
  ["instruction", "Инструкция"], ["pause", "Пауза"],
];

export class AppUI {
  readonly root: HTMLElement;
  readonly canvasHost: HTMLElement;
  readonly controls: HTMLElement;
  readonly gestureSurface: HTMLElement;
  private readonly polite: HTMLElement;
  private readonly assertive: HTMLElement;
  private readonly hud: HTMLElement;
  private readonly setup: HTMLElement;
  private readonly result: HTMLElement;
  private readonly pausePanel: HTMLElement;
  private startHandler: ((options: StartOptions) => void) | null = null;
  private restartHandler: (() => void) | null = null;
  private resetHandler: (() => void) | null = null;
  private pauseReturnFocus: HTMLElement | null = null;

  constructor(container: HTMLElement, save: SaveData) {
    const keyboardFields = bindingLabels.map(([key, label]) =>
      `<label>${label}<input class="key-binding" data-binding="${key}" value="${save.settings.keyboard[key]}" maxlength="24" spellcheck="false" aria-describedby="key-help"></label>`,
    ).join("");
    container.innerHTML = `
      <main class="app-shell">
        <header class="hero">
          <p class="eyebrow">Пространственная аудиоигра</p>
          <h1>Полуночная сортировочная</h1>
          <p>Исследуй шесть акустических зон, доставь четыре перегревающихся энергоядра и переживи финальную блокировку.</p>
        </header>
        <section id="setup" class="panel" aria-labelledby="setup-title">
          <h2 id="setup-title">Перед сменой</h2>
          <fieldset><legend>Режим управления</legend>
            <label><input type="radio" name="input-mode" value="voiceover" ${save.settings.inputMode === "voiceover" ? "checked" : ""}> VoiceOver — постоянные HTML-кнопки</label>
            <label><input type="radio" name="input-mode" value="gestures" ${save.settings.inputMode === "gestures" ? "checked" : ""}> Жесты без VoiceOver</label>
            <label><input type="radio" name="input-mode" value="keyboard" ${save.settings.inputMode === "keyboard" ? "checked" : ""}> Клавиатура</label>
          </fieldset>
          <div class="settings-grid">
            <label>Подробность<select id="verbosity"><option value="minimal">Минимальная</option><option value="normal">Обычная</option><option value="detailed">Подробная</option></select></label>
            <label><input id="speech" type="checkbox" ${save.settings.speech ? "checked" : ""}> Системная речь вне VoiceOver</label>
            <label>Громкость<input id="volume" type="range" min="0.2" max="1" step="0.05" value="${save.settings.masterVolume}"></label>
            <label>Чувствительность жестов<input id="gesture-sensitivity" type="range" min="0.6" max="1.6" step="0.1" value="${save.settings.gestureSensitivity}"></label>
            <label>Масштаб визуала<input id="visual-scale" type="range" min="0.8" max="1.4" step="0.1" value="${save.settings.visualScale}"></label>
          </div>
          <details id="keyboard-settings"><summary>Переназначение клавиатуры</summary>
            <p id="key-help">Укажи значения KeyboardEvent.code, например KeyW, ArrowUp, Space или Escape.</p>
            <div class="settings-grid">${keyboardFields}</div>
          </details>
          <p>Лучший результат: <strong id="best-score">${save.bestScore}</strong>. Завершённых смен: <strong id="completed-runs">${save.completedRuns}</strong>.</p>
          <button id="start-game" class="primary">Включить звук и начать</button>
          <button id="reset-save" class="danger-outline">Полный сброс сохранения</button>
        </section>
        <section id="game-area" class="game-area" hidden>
          <div id="canvas-host" class="canvas-host" aria-hidden="true"></div>
          <div id="gesture-surface" class="gesture-surface" tabindex="0" aria-label="Жестовая поверхность. Свайп вверх или вниз — движение, влево или вправо — поворот на девяносто градусов, касание — действие, двойное касание — бросить болт, тройное — пауза, четыре касания — инструкция, долгое касание — статус, длинный свайп вниз — экстренно остановить звук." hidden>
            <span>Жестовая поверхность</span>
          </div>
          <section id="controls" class="panel controls" aria-labelledby="controls-title">
            <h2 id="controls-title">Управление</h2>
            <div class="control-grid">
              <button data-command="forward">Вперёд</button><button data-command="fast">Быстро вперёд</button><button data-command="back">Назад</button>
              <button data-command="left">Повернуть влево</button><button data-command="right">Повернуть вправо</button>
              <button data-command="interact" class="primary">Действие</button><button data-command="special">Бросить болт</button>
              <button data-command="status">Статус</button><button data-command="instruction">Повторить инструкцию</button>
              <button data-command="pause">Пауза</button><button data-command="stop" class="danger-outline">Экстренно остановить звук</button>
            </div>
          </section>
          <section class="panel" aria-labelledby="status-title"><h2 id="status-title">Состояние</h2><div id="hud"></div></section>
        </section>
        <section id="pause-panel" class="modal panel" hidden role="dialog" aria-modal="true" aria-labelledby="pause-title">
          <h2 id="pause-title">Пауза</h2><p>Игровое время, AI и движение остановлены.</p>
          <button id="resume-game" data-command="pause" class="primary">Продолжить</button><button id="restart-from-pause">Начать заново</button>
        </section>
        <section id="result" class="modal panel" hidden role="dialog" aria-modal="true" aria-labelledby="result-title">
          <h2 id="result-title">Результат</h2><p id="result-text"></p><button id="restart-game" class="primary">Сыграть ещё раз</button>
        </section>
        <div id="live-polite" class="sr-only" aria-live="polite" aria-atomic="true"></div>
        <div id="live-assertive" class="sr-only" aria-live="assertive" aria-atomic="true"></div>
      </main>`;
    this.root = container;
    this.canvasHost = this.required("#canvas-host");
    this.controls = this.required("#controls");
    this.gestureSurface = this.required("#gesture-surface");
    this.polite = this.required("#live-polite");
    this.assertive = this.required("#live-assertive");
    this.hud = this.required("#hud");
    this.setup = this.required("#setup");
    this.result = this.required("#result");
    this.pausePanel = this.required("#pause-panel");
    this.required<HTMLSelectElement>("#verbosity").value = save.settings.verbosity;
    this.required("#start-game").addEventListener("click", () => this.startHandler?.({ settings: this.readSettings() }));
    this.required("#reset-save").addEventListener("click", () => this.resetHandler?.());
    this.required("#restart-game").addEventListener("click", () => this.restartHandler?.());
    this.required("#restart-from-pause").addEventListener("click", () => this.restartHandler?.());
  }

  onStart(handler: (options: StartOptions) => void): void { this.startHandler = handler; }
  onRestart(handler: () => void): void { this.restartHandler = handler; }
  onReset(handler: () => void): void { this.resetHandler = handler; }

  showGame(settings: PersistedSettings): void {
    this.setup.hidden = true;
    this.result.hidden = true;
    this.pausePanel.hidden = true;
    this.required("#game-area").hidden = false;
    this.controls.hidden = settings.inputMode !== "voiceover";
    this.gestureSurface.hidden = settings.inputMode !== "gestures";
    document.documentElement.style.setProperty("--visual-scale", String(settings.visualScale));
    if (settings.inputMode === "voiceover") this.controls.querySelector<HTMLButtonElement>("button[data-command='forward']")?.focus();
  }

  showSetup(save: SaveData): void {
    this.required("#game-area").hidden = true;
    this.result.hidden = true;
    this.pausePanel.hidden = true;
    this.setup.hidden = false;
    this.required("#best-score").textContent = String(save.bestScore);
    this.required("#completed-runs").textContent = String(save.completedRuns);
    this.setup.querySelector<HTMLButtonElement>("#start-game")?.focus();
    this.announce(`Сохранение сброшено. Лучший результат ${save.bestScore}.`, false);
  }

  updateHud(text: string): void { if (this.hud.textContent !== text) this.hud.textContent = text; }

  announce(text: string, assertive: boolean): void {
    const target = assertive ? this.assertive : this.polite;
    target.textContent = "";
    window.setTimeout(() => { target.textContent = text; }, 16);
  }

  showPause(): void {
    this.pauseReturnFocus = document.activeElement instanceof HTMLElement ? document.activeElement : null;
    this.pausePanel.hidden = false;
    this.required<HTMLButtonElement>("#resume-game").focus();
  }

  hidePause(): void { this.pausePanel.hidden = true; this.pauseReturnFocus?.focus(); }
  isPauseVisible(): boolean { return !this.pausePanel.hidden; }

  showResult(title: string, text: string): void {
    this.pausePanel.hidden = true;
    this.result.hidden = false;
    this.required("#result-title").textContent = title;
    this.required("#result-text").textContent = text;
    this.required<HTMLButtonElement>("#restart-game").focus();
  }

  readSettings(): PersistedSettings {
    const inputMode = this.root.querySelector<HTMLInputElement>("input[name='input-mode']:checked")?.value as PersistedSettings["inputMode"] | undefined;
    const keyboard = {} as KeyboardBindings;
    for (const [key] of bindingLabels) keyboard[key] = this.required<HTMLInputElement>(`input[data-binding='${key}']`).value.trim();
    return {
      inputMode: inputMode ?? "keyboard",
      verbosity: this.required<HTMLSelectElement>("#verbosity").value as PersistedSettings["verbosity"],
      speech: this.required<HTMLInputElement>("#speech").checked,
      masterVolume: Number(this.required<HTMLInputElement>("#volume").value),
      gestureSensitivity: Number(this.required<HTMLInputElement>("#gesture-sensitivity").value),
      visualScale: Number(this.required<HTMLInputElement>("#visual-scale").value),
      keyboard,
    };
  }

  private required<T extends HTMLElement = HTMLElement>(selector: string): T {
    const element = this.root.querySelector<T>(selector);
    if (!element) throw new Error(`Missing UI element ${selector}`);
    return element;
  }
}
