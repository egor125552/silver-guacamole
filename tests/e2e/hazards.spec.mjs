import { test, expect } from "@playwright/test";
import { action, snapshot } from "./helpers.mjs";

async function startScenario(page, scenario, timeScale = 1) {
  const errors = [];
  page.on("pageerror", (error) => errors.push(error.message));
  page.on("console", (message) => { if (message.type() === "error") errors.push(message.text()); });
  await page.goto(`?testMode=1&scenario=${scenario}&timeScale=${timeScale}`);
  await page.getByLabel("Клавиатура").check();
  await page.getByRole("button", { name: "Включить звук и начать" }).click();
  await page.waitForFunction(() => Boolean(window.__SWITCHYARD_TEST__));
  return errors;
}

for (const scenario of ["drone", "trolley"]) {
  test(`${scenario} hazard and restart through real rules`, async ({ page, browserName }) => {
    test.skip(browserName !== "chromium");
    const errors = await startScenario(page, scenario);
    await expect.poll(async () => (await snapshot(page)).phase, { timeout: 12_000 }).toBe("lost");
    await expect(page.getByRole("button", { name: "Сыграть ещё раз" })).toBeFocused();
    await page.getByRole("button", { name: "Сыграть ещё раз" }).click();
    await expect.poll(async () => (await snapshot(page)).phase).toBe("running");
    expect(errors).toEqual([]);
  });
}

test("overheating hazard uses pickup, heat, drop and damage rules", async ({ page, browserName }) => {
  test.skip(browserName !== "chromium");
  const errors = await startScenario(page, "overheat", 30);
  for (let hit = 0; hit < 3; hit += 1) {
    await action(page, "keyboard", "interact");
    await expect.poll(async () => (await snapshot(page)).carriedCore, { timeout: 2_000 }).toBe("amber");
    await expect.poll(async () => (await snapshot(page)).carriedCore, { timeout: 5_000 }).toBe(null);
  }
  await expect.poll(async () => (await snapshot(page)).phase).toBe("lost");
  await page.getByRole("button", { name: "Сыграть ещё раз" }).click();
  await expect.poll(async () => (await snapshot(page)).phase).toBe("running");
  expect(errors).toEqual([]);
});
