import { expect } from "@playwright/test";
import { action, snapshot, targets } from "./helpers.mjs";

async function moveTo(page, point) {
  await page.evaluate(([x, y]) => window.__SWITCHYARD_TEST__.moveTo(x, y), [point.x, point.y]);
}

async function useAt(page, mode, point, command = "interact") {
  await moveTo(page, point);
  await action(page, mode, command);
}

async function openDoor(page, mode, id, point) {
  await moveTo(page, point);
  if (!(await snapshot(page)).doors[id]) await action(page, mode, "interact");
  await expect.poll(async () => (await snapshot(page)).doors[id]).toBe(true);
}

export async function fastFullRun(page, mode, options = {}) {
  const t = await targets(page);
  const doors = t.doors;

  await useAt(page, mode, t.cores[0]);
  await openDoor(page, mode, "yard-north", doors["yard-north"]);
  await useAt(page, mode, t.bay);
  await expect.poll(async () => (await snapshot(page)).delivered.length).toBe(1);

  await useAt(page, mode, t.switches[0]);
  if (options.useBolt !== false) await action(page, mode, "special");
  await useAt(page, mode, t.cores[1]);
  if (options.useCooling) await moveTo(page, t.coolPads[2]);
  await useAt(page, mode, t.bay);
  await expect.poll(async () => (await snapshot(page)).delivered.length).toBe(2);

  await openDoor(page, mode, "shaft-cooling", doors["shaft-cooling"]);
  await useAt(page, mode, t.cores[2]);
  await useAt(page, mode, t.bay);
  await expect.poll(async () => (await snapshot(page)).delivered.length).toBe(3);

  await openDoor(page, mode, "corridor-gate", doors["corridor-gate"]);
  await useAt(page, mode, t.switches[1]);
  if (options.useBolt !== false) await action(page, mode, "special");
  await useAt(page, mode, t.cores[3]);
  if (options.useCooling) await moveTo(page, t.coolPads[4]);
  await useAt(page, mode, t.bay);
  await expect.poll(async () => (await snapshot(page)).phase).toBe("lockdown");

  await useAt(page, mode, t.switches[1]);
  await useAt(page, mode, t.switches[0]);
  await useAt(page, mode, t.exit);
  await expect.poll(async () => (await snapshot(page)).phase).toBe("won");
}
