import { expect } from "@playwright/test";

const labels = { keyboard: "Клавиатура", voiceover: "VoiceOver — постоянные HTML-кнопки", gestures: "Жесты без VoiceOver" };
let pointerSequence = 90;

export async function startGame(page, mode = "keyboard", extra = "") {
  const errors = [];
  page.on("pageerror", (error) => errors.push(error.message));
  page.on("console", (message) => { if (message.type() === "error") errors.push(message.text()); });
  await page.goto(`?testMode=1&scenario=playthrough&mode=${mode}${extra}`);
  await page.getByLabel(labels[mode]).check();
  await page.getByRole("button", { name: "Включить звук и начать" }).click();
  await page.waitForFunction(() => Boolean(window.__SWITCHYARD_TEST__));
  await expect.poll(async () => (await snapshot(page)).phase).toBe("running");
  return errors;
}

export async function snapshot(page) { return page.evaluate(() => window.__SWITCHYARD_TEST__.snapshot()); }
export async function targets(page) { return page.evaluate(() => window.__SWITCHYARD_TEST__.targets()); }

async function gesturePoint(page) {
  const surface = page.locator("#gesture-surface");
  const box = await surface.boundingBox();
  if (!box) throw new Error("Gesture surface is not visible");
  return { surface, x: box.x + box.width / 2, y: box.y + box.height / 2 };
}

async function dispatchTouch(surface, type, pointerId, x, y, buttons) {
  await surface.dispatchEvent(type, {
    pointerId,
    pointerType: "touch",
    isPrimary: true,
    clientX: x,
    clientY: y,
    button: 0,
    buttons,
    bubbles: true,
    cancelable: true,
  });
}

async function gestureSwipe(page, dx, dy, duration = 80) {
  const { surface, x, y } = await gesturePoint(page);
  const pointerId = pointerSequence += 1;
  await dispatchTouch(surface, "pointerdown", pointerId, x, y, 1);
  await page.waitForTimeout(duration);
  await dispatchTouch(surface, "pointerup", pointerId, x + dx, y + dy, 0);
}

async function gestureTaps(page, count, holdMs = 24, gapMs = 58) {
  const { surface, x, y } = await gesturePoint(page);
  for (let index = 0; index < count; index += 1) {
    const pointerId = pointerSequence += 1;
    await dispatchTouch(surface, "pointerdown", pointerId, x, y, 1);
    await page.waitForTimeout(holdMs);
    await dispatchTouch(surface, "pointerup", pointerId, x, y, 0);
    if (index + 1 < count) await page.waitForTimeout(gapMs);
  }
  await page.waitForTimeout(390);
}

export async function action(page, mode, command = "interact") {
  if (mode === "keyboard") {
    const keys = { interact: "Space", special: "q", status: "i", instruction: "h", pause: "Escape", stop: "Shift+Escape" };
    await page.keyboard.press(keys[command]);
  } else if (mode === "voiceover") {
    const names = { interact: "Действие", special: "Бросить болт", status: "Статус", instruction: "Повторить инструкцию", pause: "Пауза", stop: "Экстренно остановить звук" };
    await page.getByRole("button", { name: names[command], exact: true }).click();
  } else if (command === "interact") {
    await gestureTaps(page, 1);
  } else if (command === "special") {
    await gestureTaps(page, 2);
  } else if (command === "pause") {
    await gestureTaps(page, 3);
  } else if (command === "instruction") {
    await gestureTaps(page, 4);
  } else if (command === "status") {
    const { surface, x, y } = await gesturePoint(page);
    const pointerId = pointerSequence += 1;
    await dispatchTouch(surface, "pointerdown", pointerId, x, y, 1);
    await page.waitForTimeout(660);
    await dispatchTouch(surface, "pointerup", pointerId, x, y, 0);
  } else if (command === "stop") {
    await gestureSwipe(page, 0, 170, 760);
  }
  await page.waitForTimeout(80);
}
