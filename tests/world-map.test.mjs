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

const { WORLD, distance } = await loadTs("src/world/WorldMap.ts");

const entryByDrone = {
  "hangar-sentinel": "yard-north",
  "shaft-sentinel": "shaft-access",
  "cooling-listener": "cooling-access",
  "machine-sentinel": "machine-access",
};

test("zone patrols do not spawn inside their entry-door sight radius", () => {
  for (const [droneId, doorId] of Object.entries(entryByDrone)) {
    const drone = WORLD.droneSpecs.find((item) => item.id === droneId);
    const door = WORLD.doors.find((item) => item.id === doorId);
    assert.ok(drone, `missing drone ${droneId}`);
    assert.ok(door, `missing door ${doorId}`);
    assert.ok(
      distance(drone.route[0], door.position) > drone.sightRadius,
      `${droneId} starts unfairly close to ${doorId}`,
    );
  }
});
