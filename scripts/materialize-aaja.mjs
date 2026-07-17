import { spawnSync } from "node:child_process";
import { cpSync, existsSync, mkdirSync, readFileSync, rmSync, statSync, writeFileSync } from "node:fs";
import { resolve } from "node:path";

const root = resolve(import.meta.dirname, "..");
const commit = "4ad11f77b15163699d9213bc461d8aefb37c12f7";
const source = resolve(root, ".aaja-build/source");
const target = resolve(root, "vendor/aaja-package");
const targetWasm = resolve(target, "dist/wasm/audio_game_core_bg.wasm");
const targetCommit = resolve(target, "ENGINE_COMMIT");

function run(command, args, options = {}) {
  const result = spawnSync(command, args, {
    cwd: root,
    encoding: "utf8",
    stdio: "inherit",
    maxBuffer: 64 * 1024 * 1024,
    ...options,
  });
  if (result.status !== 0) throw new Error(`${command} failed with status ${result.status ?? "unknown"}`);
}

if (
  existsSync(targetWasm) &&
  statSync(targetWasm).size > 10_000 &&
  existsSync(targetCommit) &&
  readFileSync(targetCommit, "utf8").trim() === commit
) {
  console.log(`Aaja ${commit} is already materialized.`);
  process.exit(0);
}

rmSync(resolve(root, ".aaja-build"), { recursive: true, force: true });
mkdirSync(source, { recursive: true });
run("git", ["init", "-q"], { cwd: source });
run("git", ["remote", "add", "origin", "https://github.com/egor125552/rust-Aaja-game-engine-web.git"], { cwd: source });
run("git", ["fetch", "--depth", "1", "origin", commit], { cwd: source });
run("git", ["checkout", "--detach", "FETCH_HEAD"], { cwd: source });
run("npm", ["ci"], { cwd: source });
run("npm", ["run", "test:rust"], { cwd: source });
run("npm", ["run", "test:ts"], { cwd: source });
run("npm", ["run", "build"], { cwd: source });
run("npm", ["run", "test:wasm"], { cwd: source });

const builtPackage = resolve(source, "packages/engine");
const builtWasm = resolve(builtPackage, "dist/wasm/audio_game_core_bg.wasm");
if (!existsSync(builtWasm) || statSync(builtWasm).size < 10_000) throw new Error("Aaja build did not emit a plausible WASM module.");
rmSync(target, { recursive: true, force: true });
mkdirSync(target, { recursive: true });
cpSync(resolve(builtPackage, "dist"), resolve(target, "dist"), { recursive: true });
cpSync(resolve(builtPackage, "package.json"), resolve(target, "package.json"));
writeFileSync(targetCommit, `${commit}\n`);
rmSync(resolve(root, ".aaja-build"), { recursive: true, force: true });
console.log(`Materialized Aaja ${commit}.`);
