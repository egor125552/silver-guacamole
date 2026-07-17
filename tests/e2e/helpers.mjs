import { expect } from "@playwright/test";

const labels = { keyboard: "Клавиатура", voiceover: "VoiceOver — постоянные HTML-кнопки", gestures: "Жесты без VoiceOver" };
const normalize = (angle) => Math.atan2(Math.sin(angle), Math.cos(angle));

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

async function gestureSwipe(page, dx, dy, duration = 80) {
  const surface = page.locator("#gesture-surface");
  const box = await surface.boundingBox();
  if (!box) throw new Error("Gesture surface is not visible");
  const x = box.x + box.width / 2;
  const y = box.y + box.height / 2;
  const pointerId = 91;
  await surface.dispatchEvent("pointerdown", {
    pointerId,
    pointerType: "touch",
    isPrimary: true,
    clientX: x,
    clientY: y,
    button: 0,
    buttons: 1,
    bubbles: true,
    cancelable: true,
  });
  await page.waitForTimeout(duration);
  await surface.dispatchEvent("pointerup", {
    pointerId,
    pointerType: "touch",
    isPrimary: true,
    clientX: x + dx,
    clientY: y + dy,
    button: 0,
    buttons: 0,
    bubbles: true,
    cancelable: true,
  });
}

async function turnToward(page, mode, delta) {
  const direction = delta > 0 ? 1 : -1;
  if (mode === "keyboard") {
    const key = direction > 0 ? "KeyD" : "KeyA";
    const duration = Math.max(20, Math.min(650, Math.round(Math.abs(delta) / 0.0026)));
    await page.keyboard.press(key, { delay: duration });
  } else if (mode === "voiceover") {
    await page.getByRole("button", { name: direction > 0 ? "Повернуть вправо" : "Повернуть влево", exact: true }).click();
  } else {
    await gestureSwipe(page, direction > 0 ? 80 : -80, 0);
  }
  await page.waitForTimeout(35);
}

async function orient(page, mode, desired) {
  for (let attempt = 0; attempt < 20; attempt += 1) {
    const state = await snapshot(page);
    const delta = normalize(desired - state.angle);
    if (Math.abs(delta) < 0.12) return;
    await turnToward(page, mode, delta);
  }
  const state = await snapshot(page);
  if (Math.abs(normalize(desired - state.angle)) >= 0.2) throw new Error(`Could not orient ${mode}`);
}

async function forwardStep(page, mode) {
  if (mode === "keyboard") {
    await page.keyboard.press("KeyW", { delay: 500 });
  } else if (mode === "voiceover") {
    await page.getByRole("button", { name: "Вперёд", exact: true }).click();
    await page.waitForTimeout(540);
  } else {
    await gestureSwipe(page, 0, -82);
    await page.waitForTimeout(550);
  }
  await page.waitForTimeout(45);
}

export async function navigate(page, mode, destination, tolerance = 54) {
  for (let attempt = 0; attempt < 150; attempt += 1) {
    const state = await snapshot(page);
    if (state.phase === "lost") throw new Error(`Lost while navigating to ${destination.x},${destination.y}`);
    if (Math.hypot(state.player.x - destination.x, state.player.y - destination.y) <= tolerance) return;
    const path = await page.evaluate(([x, y]) => window.__SWITCHYARD_TEST__.planPath(x, y), [destination.x, destination.y]);
    if (!Array.isArray(path) || path.length === 0) throw new Error(`No A* path to ${destination.x},${destination.y}`);
    const waypoint = path.length > 1 ? path[1] : destination;
    const dx = waypoint.x - state.player.x;
    const dy = waypoint.y - state.player.y;
    const desired = Math.abs(dx) >= Math.abs(dy) ? (dx >= 0 ? 0 : Math.PI) : (dy >= 0 ? Math.PI / 2 : -Math.PI / 2);
    await orient(page, mode, desired);
    await forwardStep(page, mode);
  }
  throw new Error(`Navigation did not converge to ${destination.x},${destination.y}`);
}

export async function action(page, mode, command = "interact") {
  if (mode === "keyboard") {
    const keys = { interact: "Space", special: "KeyQ", status: "KeyI", instruction: "KeyH", pause: "Escape", stop: "Shift+Escape" };
    await page.keyboard.press(keys[command]);
  } else if (mode === "voiceover") {
    const names = { interact: "Действие", special: "Бросить болт", status: "Статус", instruction: "Повторить инструкцию", pause: "Пауза", stop: "Экстренно остановить звук" };
    await page.getByRole("button", { name: names[command], exact: true }).click();
  } else {
    const surface = page.locator("#gesture-surface");
    const box = await surface.boundingBox();
    if (!box) throw new Error("Gesture surface is not visible");
    const x = box.x + box.width / 2, y = box.y + box.height / 2;
    if (command === "interact") { await page.mouse.click(x, y); await page.waitForTimeout(380); }
    else if (command === "special") { await page.mouse.click(x, y); await page.waitForTimeout(70); await page.mouse.click(x, y); await page.waitForTimeout(380); }
    else if (command === "pause") { for (let i = 0; i < 3; i += 1) { await page.mouse.click(x, y); await page.waitForTimeout(55); } await page.waitForTimeout(380); }
    else if (command === "instruction") { for (let i = 0; i < 4; i += 1) { await page.mouse.click(x, y); await page.waitForTimeout(50); } await page.waitForTimeout(380); }
    else if (command === "status") { await page.mouse.move(x, y); await page.mouse.down(); await page.waitForTimeout(650); await page.mouse.up(); await page.waitForTimeout(80); }
    else if (command === "stop") { await page.mouse.move(x, y - 60); await page.mouse.down(); await page.waitForTimeout(760); await page.mouse.move(x, y + 90, { steps: 5 }); await page.mouse.up(); await page.waitForTimeout(80); }
  }
  await page.waitForTimeout(80);
}

async function useAt(page, mode, point, command = "interact") {
  await navigate(page, mode, point);
  await action(page, mode, command);
}

export async function fullRun(page, mode, options = {}) {
  const t = await targets(page);
  const doors = t.doors;
  await useAt(page, mode, t.cores[0]);
  await useAt(page, mode, doors["yard-north"]);
  await useAt(page, mode, t.bay);

  await useAt(page, mode, t.switches[0]);
  if (options.useBolt !== false) await action(page, mode, "special");
  await useAt(page, mode, t.cores[1]);
  if (options.useCooling) await navigate(page, mode, t.coolPads[2]);
  await useAt(page, mode, t.bay);

  await useAt(page, mode, doors["shaft-cooling"]);
  await useAt(page, mode, t.cores[2]);
  await useAt(page, mode, t.bay);

  await useAt(page, mode, doors["corridor-gate"]);
  await useAt(page, mode, t.switches[1]);
  if (options.useBolt !== false) await action(page, mode, "special");
  await useAt(page, mode, t.cores[3]);
  if (options.useCooling) await navigate(page, mode, t.coolPads[4]);
  await useAt(page, mode, t.bay);
  await expect.poll(async () => (await snapshot(page)).phase, { timeout: 10_000 }).toBe("lockdown");

  await useAt(page, mode, t.switches[1]);
  await useAt(page, mode, t.switches[0]);
  await useAt(page, mode, t.exit);
  await expect.poll(async () => (await snapshot(page)).phase, { timeout: 10_000 }).toBe("won");
}
