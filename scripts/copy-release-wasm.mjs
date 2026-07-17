import { copyFileSync, existsSync, mkdirSync, statSync } from "node:fs";
import { resolve } from "node:path";

const root = resolve(import.meta.dirname, "..");
const source = resolve(root, "vendor/aaja-package/dist/wasm/audio_game_core_bg.wasm");
const targetDirectory = resolve(root, "dist/assets");
const target = resolve(targetDirectory, "audio_game_core_bg.wasm");
if (!existsSync(source) || statSync(source).size < 10_000) throw new Error("Pinned Aaja WASM source is missing");
mkdirSync(targetDirectory, { recursive: true });
copyFileSync(source, target);
if (!existsSync(target) || statSync(target).size !== statSync(source).size) throw new Error("Aaja WASM was not copied into the Pages artifact");
console.log(`Copied Aaja WASM to dist/assets/audio_game_core_bg.wasm (${statSync(target).size} bytes).`);
