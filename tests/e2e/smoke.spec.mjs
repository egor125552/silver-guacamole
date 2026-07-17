import { test, expect } from "@playwright/test";
import { action, snapshot, startGame } from "./helpers.mjs";

test("loads engine, supports status, pause, blur reset and audio recovery", async ({ page, browserName }) => {
  const mode = browserName === "webkit" ? "voiceover" : "keyboard";
  const errors = await startGame(page, mode);
  await action(page, mode, "status");
  await action(page, mode, "pause");
  await expect(page.locator("#pause-panel")).toBeVisible();
  await page.getByRole("button", { name: "Продолжить" }).click();
  await expect(page.locator("#pause-panel")).toBeHidden();
  await page.keyboard.down("ArrowUp");
  await page.waitForTimeout(120);
  await page.evaluate(() => window.dispatchEvent(new Event("blur")));
  await page.keyboard.up("ArrowUp");
  expect((await snapshot(page)).heldCommands).toBe(0);
  await action(page, mode, "stop");
  await action(page, mode, "status");
  expect((await snapshot(page)).errors).toBe(0);
  expect(errors).toEqual([]);
});

test("VoiceOver buttons keep focus after repeated commands", async ({ page }) => {
  const errors = await startGame(page, "voiceover");
  const forward = page.getByRole("button", { name: "Вперёд", exact: true });
  await forward.focus();
  await forward.click();
  await page.waitForTimeout(80);
  await expect(forward).toBeFocused();
  const status = page.getByRole("button", { name: "Статус", exact: true });
  await status.click();
  await expect(status).toBeFocused();
  expect(errors).toEqual([]);
});

test("gesture pointer cancellation clears held state without moving focus", async ({ page }) => {
  const errors = await startGame(page, "gestures");
  await expect(page.locator("#gesture-surface")).not.toBeFocused();
  await page.locator("#gesture-surface").dispatchEvent("pointerdown", { pointerId: 7, clientX: 120, clientY: 220, pointerType: "touch" });
  await page.locator("#gesture-surface").dispatchEvent("pointercancel", { pointerId: 7, clientX: 120, clientY: 220, pointerType: "touch" });
  expect((await snapshot(page)).heldCommands).toBe(0);
  expect(errors).toEqual([]);
});
