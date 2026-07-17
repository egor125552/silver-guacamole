import { existsSync, readFileSync } from "node:fs";
import { resolve } from "node:path";
const root = resolve(import.meta.dirname, "..");
const expected = "4ad11f77b15163699d9213bc461d8aefb37c12f7";
const commitFile = resolve(root, "vendor/aaja-package/ENGINE_COMMIT");
const wasm = resolve(root, "vendor/aaja-package/dist/wasm/audio_game_core_bg.wasm");
if (process.env.AAJA_ALLOW_MOCK === "1") { console.warn("Mock Aaja allowed for local logic-only build."); process.exit(0); }
if (!existsSync(commitFile) || readFileSync(commitFile, "utf8").trim() !== expected) throw new Error("Release Aaja package was not materialized from the pinned commit.");
if (!existsSync(wasm)) throw new Error("Pinned Aaja WASM is missing from the release package.");
console.log(`Verified Aaja release package at ${expected}.`);
