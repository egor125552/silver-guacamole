import { test, expect } from "@playwright/test";
import { fullRun, snapshot, startGame } from "./helpers.mjs";

for (const mode of ["keyboard", "voiceover", "gestures"]) {
  test(`complete input-driven run through ${mode}`, async ({ page, browserName }) => {
    test.skip(browserName !== "chromium", "Full adapter playthroughs run in Chromium; cross-browser smoke is separate.");
    const errors = await startGame(page, mode);
    await fullRun(page, mode, { useBolt: true, useCooling: true });
    const state = await snapshot(page);
    expect(state.delivered).toHaveLength(4);
    expect(state.activeSources).toBeLessThanOrEqual(32);
    expect(state.heldCommands).toBe(0);
    expect(state.errors).toBe(0);
    expect(errors).toEqual([]);
    await page.getByRole("button", { name: "Сыграть ещё раз" }).click();
    await expect.poll(async () => (await snapshot(page)).phase).toBe("running");
  });
}

test("win persists, reloads, replays and full reset clears versioned save", async ({ page, browserName }) => {
  test.skip(browserName !== "chromium");
  await startGame(page, "keyboard");
  await fullRun(page, "keyboard", { useBolt: true, useCooling: true });
  const saved = await page.evaluate(() => JSON.parse(localStorage.getItem("midnight-switchyard-save-v2")));
  expect(saved.version).toBe(2);
  expect(saved.completedRuns).toBe(1);
  expect(saved.bestScore).toBeGreaterThan(0);
  await page.reload();
  await expect(page.getByText(/Завершённых смен:/)).toContainText("1");
  await page.evaluate(() => localStorage.setItem("midnight-switchyard-save-v2", '{"version":2,"bestScore":null,"completedRuns":-5,"settings":{"inputMode":"broken","masterVolume":"NaN","keyboard":{"forward":"<bad>"}}}'));
  await page.reload();
  await expect(page.getByLabel("Клавиатура")).toBeChecked();
  await page.getByRole("button", { name: "Полный сброс сохранения" }).click();
  expect(await page.evaluate(() => localStorage.getItem("midnight-switchyard-save-v2"))).toBeNull();
});
