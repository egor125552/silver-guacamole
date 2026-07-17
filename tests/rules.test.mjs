import test from "node:test";
import assert from "node:assert/strict";
import ts from "typescript";
import { readFileSync } from "node:fs";
import { resolve } from "node:path";

async function loadTs(path) {
  const source = readFileSync(resolve(path), "utf8");
  const output = ts.transpileModule(source, {
    compilerOptions: { target: ts.ScriptTarget.ES2022, module: ts.ModuleKind.ES2022, useDefineForClassFields: true },
  }).outputText;
  return import(`data:text/javascript;base64,${Buffer.from(output).toString("base64")}`);
}

const { GameRules } = await loadTs("src/game/rules.ts");
const coreIds = ["amber", "cobalt", "violet", "emerald"];

function deliverAll(rules) {
  for (const id of coreIds) {
    assert.equal(rules.pickCore(id), true);
    assert.equal(rules.deliverCore(), id);
  }
}

test("four real deliveries start lockdown and require both switches", () => {
  const rules = new GameRules(); rules.start(); deliverAll(rules);
  assert.equal(rules.phase, "lockdown");
  assert.equal(rules.canExit(), false);
  rules.toggleSwitch("north"); assert.equal(rules.canExit(), false);
  rules.toggleSwitch("south"); assert.equal(rules.canExit(), true);
  const score = rules.win();
  assert.ok(score >= 100); assert.equal(rules.phase, "won");
});

test("accessible carry window lasts about a minute before overheat", () => {
  const rules = new GameRules(); rules.start(); rules.pickCore("amber");
  rules.update(55_000, false, { x: 200, y: 300 });
  assert.equal(rules.carriedCore, "amber");
  assert.equal(rules.health, 3);
  rules.update(6_000, false, { x: 200, y: 300 });
  assert.equal(rules.carriedCore, null);
  assert.equal(rules.health, 2);
  assert.ok(rules.events.some((event) => event.type === "core-dropped"));
});

test("cooling reduces heat and damage cooldown prevents frame-rate deaths", () => {
  const rules = new GameRules(); rules.start(); rules.pickCore("amber");
  rules.update(10_000, false, { x: 0, y: 0 });
  const hot = rules.coreHeat;
  rules.update(2_000, true, { x: 0, y: 0 });
  assert.ok(rules.coreHeat < hot);
  rules.damage(1, "test"); rules.damage(1, "test");
  assert.equal(rules.health, 2);
  rules.update(1_051, true, { x: 0, y: 0 }); rules.damage(1, "test");
  assert.equal(rules.health, 1);
});

test("lockdown timeout loses without an invisible extra condition", () => {
  const rules = new GameRules(); rules.start(); deliverAll(rules);
  rules.update(180_001, false, { x: 0, y: 0 });
  assert.equal(rules.phase, "lost");
  assert.ok(rules.events.some((event) => event.type === "lose"));
});

test("pause resumes to the correct phase", () => {
  const rules = new GameRules(); rules.start(); rules.pause(true); assert.equal(rules.phase, "paused");
  rules.pause(false); assert.equal(rules.phase, "running");
  deliverAll(rules);
  rules.pause(true); rules.pause(false); assert.equal(rules.phase, "lockdown");
});

test("limited recovery resources are one-shot and bounded", () => {
  const rules = new GameRules(); rules.start();
  rules.damage(1, "test"); rules.update(1_051, false, { x: 0, y: 0 });
  assert.equal(rules.repair("station-a"), true);
  assert.equal(rules.repair("station-a"), false);
  assert.equal(rules.openLocker("bolts-a", "bolts"), true);
  assert.equal(rules.maxBolts, 6);
  assert.equal(rules.openLocker("bolts-a", "bolts"), false);
});
