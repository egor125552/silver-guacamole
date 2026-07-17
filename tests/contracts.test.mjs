import test from "node:test";
import assert from "node:assert/strict";
import { existsSync, readFileSync, statSync } from "node:fs";

const read = (path) => readFileSync(path, "utf8");

test("all input adapters emit the shared command model", () => {
  const command = read("src/game/commands.ts");
  for (const type of ["move", "turn", "interact", "special", "status", "instruction", "pause", "emergency-stop"]) assert.match(command, new RegExp(type));
  for (const file of ["src/input/KeyboardInput.ts", "src/input/GestureInput.ts", "src/input/VoiceOverInput.ts"]) assert.match(read(file), /InputHub/);
});

test("VoiceOver controls are native, stable and have emergency actions", () => {
  const ui = read("src/ui/AppUI.ts");
  assert.match(ui, /<button data-command="forward">/);
  assert.match(ui, /data-command="interact"/);
  assert.match(ui, /data-command="stop"/);
  assert.doesNotMatch(ui, /role="button"/);
  assert.match(read("src/ui/styles.css"), /min-height:\s*44px/);
});

test("release tree requires the pinned real Aaja WASM package", () => {
  assert.match(read("scripts/verify-release-engine.mjs"), /audio_game_core_bg\.wasm/);
  assert.match(read("scripts/verify-release-engine.mjs"), /4ad11f77b15163699d9213bc461d8aefb37c12f7/);
  assert.match(read("scripts/copy-release-wasm.mjs"), /dist\/assets/);
  const wasm = "vendor/aaja-package/dist/wasm/audio_game_core_bg.wasm";
  assert.doesNotMatch(read("vendor/aaja-package/dist/index.js"), /MOCK-NOT-FOR-RELEASE|cannot be used in a release build/i);
  if (existsSync(wasm)) assert.ok(statSync(wasm).size > 10_000);
  else {
    assert.match(read("scripts/materialize-aaja.mjs"), /rust-Aaja-game-engine-web/);
    assert.match(read("scripts/materialize-aaja.mjs"), /4ad11f77b15163699d9213bc461d8aefb37c12f7/);
  }
});

test("Phaser owns the loop and Web Audio is disabled in Phaser", () => {
  const bootstrap = read("src/bootstrap.ts");
  assert.match(bootstrap, /physics:\s*\{ default: "arcade"/);
  assert.match(bootstrap, /disableWebAudio: true/);
  assert.match(bootstrap, /noAudio: true/);
});


test("CI is read-only and E2E does not set coordinates or victory directly", () => {
  for (const workflow of [".github/workflows/ci.yml", ".github/workflows/pages.yml"]) {
    const text = read(workflow);
    assert.doesNotMatch(text, /contents:\s*write|\bgit\s+(commit|push)\b|^\s*issues:|^\s*schedule:/m);
  }
  const helper = read("tests/e2e/helpers.mjs");
  assert.doesNotMatch(helper, /navigateTo|setPosition\(|phase\s*=\s*["']won/);
  assert.match(helper, /keyboard\.down|mouse\.down|getByRole\("button"/);
});
