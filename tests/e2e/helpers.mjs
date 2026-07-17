import { expect } from "@playwright/test";

const labels = { keyboard: "Клавиатура", voiceover: "VoiceOver — постоянные HTML-кнопки", gestures: "Жесты без VoiceOver" };
const normalize = (angle) => Math.atan2(Math.sin(angle), Math.cos(angle));
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

async function turnToward(page, mode, delta) {
  const direction = delta > 0 ? 1 : -1;
  if (mode === "keyboard") {
    const key = direction > 0 ? "d" : "a";
    const duration = Math.max(24, Math.min(620, Math.round(Math.abs(delta) / 0.0026)));
    await page.keyboard.down(key);
    await page.waitForTimeout(duration);
    await page.keyboard.up(key);
  } else if (mode === "voiceover") {
    await page.getByRole("button", { name: direction > 0 ? "Повернуть вправо" : "Повернуть влево", exact: true }).click();
  } else {
    await gestureSwipe(page, direction > 0 ? 80 : -80, 0);
  }
  await page.waitForTimeout(25);
}

async function orient(page, mode, desired) {
  for (let attempt = 0; attempt < 24; attempt += 1) {
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
    await page.keyboard.down("Shift");
    await page.keyboard.down("w");
    await page.waitForTimeout(108);
    await page.keyboard.up("w");
    await page.keyboard.up("Shift");
  } else if (mode === "voiceover") {
    await page.getByRole("button", { name: "Быстро вперёд", exact: true }).click();
    await page.waitForTimeout(145);
  } else {
    await gestureSwipe(page, 0, -82);
    await page.waitForTimeout(215);
  }
  await page.waitForTimeout(25);
}

async function backwardAlignmentStep(page, mode) {
  if (mode === "keyboard") {
    await page.keyboard.down("s");
    await page.waitForTimeout(172);
    await page.keyboard.up("s");
  } else if (mode === "voiceover") {
    await page.getByRole("button", { name: "Назад", exact: true }).click();
    await page.waitForTimeout(325);
  } else {
    await gestureSwipe(page, 0, 82);
    await page.waitForTimeout(325);
  }
  await page.waitForTimeout(25);
}

export async function navigate(page, mode, destination, tolerance = 54) {
  for (let attempt = 0; attempt < 220; attempt += 1) {
    const state = await snapshot(page);
    if (state.phase === "lost") throw new Error(`Lost while navigating to ${destination.x},${destination.y}; player=${state.player.x.toFixed(1)},${state.player.y.toFixed(1)} health=${state.health} delivered=${state.delivered.length}`);
    if (Math.hypot(state.player.x - destination.x, state.player.y - destination.y) <= tolerance) return;
    const path = await page.evaluate(([x, y]) => window.__SWITCHYARD_TEST__.planPath(x, y), [destination.x, destination.y]);
    if (!Array.isArray(path) || path.length === 0) throw new Error(`No A* path to ${destination.x},${destination.y}`);
    const waypoint = path.length > 1 ? path[1] : destination;
    const currentCenter = path[0];
    const horizontalLeg = Math.abs(waypoint.x - currentCenter.x) >= Math.abs(waypoint.y - currentCenter.y);
    const crossOffset = horizontalLeg ? state.player.y - currentCenter.y : state.player.x - currentCenter.x;
    if (path.length > 1 && Math.abs(crossOffset) > 16) {
      const correction = horizontalLeg
        ? (crossOffset > 0 ? -Math.PI / 2 : Math.PI / 2)
        : (crossOffset > 0 ? Math.PI : 0);
      await orient(page, mode, normalize(correction + Math.PI));
      await backwardAlignmentStep(page, mode);
      continue;
    }
    const dx = waypoint.x - state.player.x;
    const dy = waypoint.y - state.player.y;
    const desired = Math.abs(dx) >= Math.abs(dy) ? (dx >= 0 ? 0 : Math.PI) : (dy >= 0 ? Math.PI / 2 : -Math.PI / 2);
    await orient(page, mode, desired);
    await forwardStep(page, mode);
  }
  const state = await snapshot(page);
  throw new Error(`Navigation did not converge to ${destination.x},${destination.y}; player=${state.player.x.toFixed(1)},${state.player.y.toFixed(1)} angle=${state.angle.toFixed(3)} phase=${state.phase}`);
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
  await page.waitForTimeout(70);
}

async function useAt(page, mode, point, command = "interact") {
  await navigate(page, mode, point);
  await action(page, mode, command);
}

async function openDoor(page, mode, id, point) {
  await navigate(page, mode, point);
  if (!(await snapshot(page)).doors[id]) await action(page, mode, "interact");
  await expect.poll(async () => (await snapshot(page)).doors[id], { timeout: 3_000 }).toBe(true);
}

async function crossAndCloseVerticalDoor(page, mode, id, point) {
  await openDoor(page, mode, id, point);
  await navigate(page, mode, { x: point.x + 64, y: point.y }, 18);
  await action(page, mode, "interact");
  await expect.poll(async () => (await snapshot(page)).doors[id], { timeout: 3_000 }).toBe(false);
}

async function distractBehindClosedGate(page, mode) {
  await orient(page, mode, Math.PI);
  await action(page, mode, "special");
}

async function coolCarriedCore(page, mode, point) {
  await navigate(page, mode, point, 42);
  for (let attempt = 0; attempt < 45; attempt += 1) {
    const state = await snapshot(page);
    if (!state.carriedCore) throw new Error(`Core was lost before cooling at ${point.x},${point.y}`);
    if (state.heat <= 18) return;
    await page.waitForTimeout(200);
  }
  const state = await snapshot(page);
  throw new Error(`Cooling did not reduce heat at ${point.x},${point.y}; heat=${state.heat.toFixed(1)} mode=${mode}`);
}

export async function fullRun(page, mode, options = {}) {
  const t = await targets(page);
  const doors = t.doors;
  await useAt(page, mode, t.cores[0]);
  await crossAndCloseVerticalDoor(page, mode, "yard-north", doors["yard-north"]);
  if (options.useBolt !== false) await distractBehindClosedGate(page, mode);
  if (options.useCooling) await coolCarriedCore(page, mode, t.coolPads[1]);
  await useAt(page, mode, t.bay);

  await useAt(page, mode, t.switches[0]);
  if (options.useBolt !== false) await action(page, mode, "special");
  await useAt(page, mode, t.repairs[1]);
  await useAt(page, mode, t.cores[1]);
  if (options.useCooling) await coolCarriedCore(page, mode, t.coolPads[1]);
  await useAt(page, mode, t.bay);

  await openDoor(page, mode, "shaft-cooling", doors["shaft-cooling"]);
  await useAt(page, mode, t.lockers[1]);
  await useAt(page, mode, t.cores[2]);
  if (options.useCooling) await coolCarriedCore(page, mode, t.coolPads[2]);
  await useAt(page, mode, t.bay);

  await openDoor(page, mode, "corridor-gate", doors["corridor-gate"]);
  await useAt(page, mode, t.switches[1]);
  if (options.useBolt !== false) await action(page, mode, "special");
  await useAt(page, mode, t.cores[3]);
  if (options.useCooling) await coolCarriedCore(page, mode, t.coolPads[4]);
  await useAt(page, mode, t.bay);
  await expect.poll(async () => (await snapshot(page)).phase, { timeout: 10_000 }).toBe("lockdown");

  await useAt(page, mode, t.switches[1]);
  await useAt(page, mode, t.switches[0]);
  await useAt(page, mode, t.exit);
  await expect.poll(async () => (await snapshot(page)).phase, { timeout: 10_000 }).toBe("won");
}
