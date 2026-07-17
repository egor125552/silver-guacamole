import test from "node:test";
import assert from "node:assert/strict";
import ts from "typescript";
import { readFileSync } from "node:fs";
import { resolve } from "node:path";

function transpile(path, replacements = {}) {
  let source = readFileSync(resolve(path), "utf8");
  for (const [from, to] of Object.entries(replacements)) source = source.replaceAll(from, to);
  return ts.transpileModule(source, { compilerOptions: { target: ts.ScriptTarget.ES2022, module: ts.ModuleKind.ES2022 } }).outputText;
}

const worldSource = transpile("src/world/WorldMap.ts", { 'import type {\n  DoorSpec,\n  GridCell,\n  Point,\n  Rect,\n  WorldSpec,\n  ZoneName,\n} from "../game/types";': '' });
const worldUrl = `data:text/javascript;base64,${Buffer.from(worldSource).toString("base64")}`;
let pathSource = transpile("src/game/pathfinding.ts");
pathSource = pathSource.replace('from "../world/WorldMap"', `from "${worldUrl}"`).replace('import type { GridCell, Point } from "./types";\n', '');
const { findGridPath } = await import(`data:text/javascript;base64,${Buffer.from(pathSource).toString("base64")}`);

test("A* routes around a wall through the only gap", () => {
  const blocked = new Set(["2,0", "2,1", "2,3", "2,4"]);
  const path = findGridPath({ col: 0, row: 2 }, { col: 4, row: 2 }, { blocked, width: 5, height: 5 });
  assert.deepEqual(path[0], { col: 0, row: 2 });
  assert.deepEqual(path.at(-1), { col: 4, row: 2 });
  assert.ok(path.some((cell) => cell.col === 2 && cell.row === 2));
  assert.equal(path.some((cell) => blocked.has(`${cell.col},${cell.row}`)), false);
});

test("A* returns no route for a sealed target", () => {
  const blocked = new Set(["1,0", "0,1", "1,1"]);
  assert.deepEqual(findGridPath({ col: 0, row: 0 }, { col: 2, row: 2 }, { blocked, width: 3, height: 3 }), []);
});

test("A* path never oscillates between adjacent cells", () => {
  const path = findGridPath({ col: 0, row: 0 }, { col: 5, row: 5 }, { blocked: new Set(), width: 6, height: 6 });
  for (let index = 2; index < path.length; index += 1) assert.notDeepEqual(path[index], path[index - 2]);
});
