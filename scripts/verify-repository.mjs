import { existsSync, readFileSync, readdirSync, statSync } from "node:fs";
import { resolve } from "node:path";

const root = resolve(import.meta.dirname, "..");
const required = [
  "src/bootstrap.ts", "src/scenes/GameScene.ts", "src/game/rules.ts", "src/world/WorldMap.ts",
  "src/entities/PlayerController.ts", "src/entities/DroneController.ts", "src/entities/CoreManager.ts",
  "src/game/noise.ts", "src/game/pathfinding.ts", "src/input/KeyboardInput.ts", "src/input/GestureInput.ts",
  "src/input/VoiceOverInput.ts", "src/audio/AajaAudioAdapter.ts", "src/audio/AudioScene.ts",
  "src/audio/GameAudioController.ts", "src/save/SaveStore.ts", "src/ui/AppUI.ts", "src/testing/TestBridge.ts",
  "tests/e2e/helpers.mjs", "tests/e2e/playthrough.spec.mjs", "tests/e2e/hazards.spec.mjs",
  ".github/workflows/ci.yml", ".github/workflows/pages.yml",
];
for (const path of required) {
  if (!existsSync(resolve(root, path))) throw new Error(`Required ordinary source file is missing: ${path}`);
}
for (const forbidden of [
  "bootstrap",
  ".github/workflows/bootstrap.yml",
  ".github/workflows/materialize.yml",
  "tests/e2e/fast-route.mjs",
]) {
  if (existsSync(resolve(root, forbidden))) throw new Error(`Forbidden bootstrap, materialization or relocation path exists: ${forbidden}`);
}

const walk = (directory) => readdirSync(directory, { withFileTypes: true }).flatMap((entry) => {
  const path = resolve(directory, entry.name);
  return entry.isDirectory() ? walk(path) : [path];
});
const read = (path) => readFileSync(resolve(root, path), "utf8");

for (const file of walk(resolve(root, ".github/workflows"))) {
  const text = readFileSync(file, "utf8");
  if (/contents:\s*write/.test(text) || /\bgit\s+(commit|push)\b/.test(text) || /^\s*issues:/m.test(text) || /^\s*schedule:/m.test(text)) {
    throw new Error(`Workflow may not mutate repository or use hidden triggers: ${file}`);
  }
}

const sourceText = walk(resolve(root, "src"))
  .filter((file) => file.endsWith(".ts"))
  .map((file) => readFileSync(file, "utf8"))
  .join("\n");
if (/new\s+(?:window\.)?(?:AudioContext|webkitAudioContext)\s*\(/.test(sourceText)) {
  throw new Error("A second AudioContext was found outside Aaja");
}

const e2eText = walk(resolve(root, "tests/e2e"))
  .filter((file) => file.endsWith(".mjs"))
  .map((file) => readFileSync(file, "utf8"))
  .join("\n");
const bridgeText = [read("src/testing/TestBridge.ts"), read("src/scenes/GameSceneTesting.ts")].join("\n");
const runtimeText = read("src/scenes/GameSceneRuntime.ts");
const playthroughText = read("tests/e2e/playthrough.spec.mjs");
const helperText = read("tests/e2e/helpers.mjs");

if (/\bmoveTo\b|\bfastFullRun\b|setPosition\s*\(|playerBody\.reset\s*\(|phase\s*=\s*["']won/.test(`${e2eText}\n${bridgeText}`)) {
  throw new Error("Browser verification contains direct relocation or win-state mutation");
}
if (/testScenario\s*===\s*["']playthrough["']/.test(runtimeText)) {
  throw new Error("Runtime hazards may not be disabled for the playthrough scenario");
}
if (!/\bfullRun\b/.test(playthroughText) || /fast-route|fastFullRun/.test(playthroughText)) {
  throw new Error("Full playthrough spec must use the command-driven helper");
}
if (!/export\s+async\s+function\s+navigate\b/.test(helperText) || !/page\.keyboard\.(?:down|press)|getByRole\(|dispatchEvent\(/.test(helperText)) {
  throw new Error("E2E helper does not expose real command-driven navigation");
}

const manifest = JSON.parse(read("assets/audio/manifest.json"));
if (manifest.assets?.length !== 31) throw new Error("Production audio manifest must contain 31 assets");
const wasm = resolve(root, "vendor/aaja-package/dist/wasm/audio_game_core_bg.wasm");
if (!existsSync(wasm) || statSync(wasm).size < 10_000) throw new Error("Materialized Aaja WASM is missing or implausibly small");
const engineCommit = read("vendor/aaja-package/ENGINE_COMMIT").trim();
if (engineCommit !== "4ad11f77b15163699d9213bc461d8aefb37c12f7") throw new Error("Unexpected Aaja engine commit");
const engineJs = read("vendor/aaja-package/dist/index.js");
if (/MOCK-NOT-FOR-RELEASE|cannot be used in a release build/i.test(engineJs)) throw new Error("Development Aaja mock is present in release tree");
const packageJson = JSON.parse(read("package.json"));
for (const [name, version] of Object.entries({ phaser: "4.2.1", "@playwright/test": "1.61.1", typescript: "5.8.3", vite: "8.1.5" })) {
  const actual = packageJson.dependencies?.[name] ?? packageJson.devDependencies?.[name];
  if (actual !== version) throw new Error(`${name} must be pinned to ${version}, got ${actual}`);
}
console.log("Verified normal repository structure, read-only workflows, command-driven E2E without relocation or disabled hazards, single Aaja audio graph, 31 licensed assets and pinned versions.");
