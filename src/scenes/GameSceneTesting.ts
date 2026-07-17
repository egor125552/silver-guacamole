import Phaser from "phaser";
import type { AajaAudioAdapter } from "../audio/AajaAudioAdapter";
import { GameAudioController } from "../audio/GameAudioController";
import type { AnnouncementQueue } from "../audio/AnnouncementQueue";
import type { AudioScene } from "../audio/AudioScene";
import type { GameDiagnostics } from "../diagnostics/Diagnostics";
import { CoreManager } from "../entities/CoreManager";
import { DroneController } from "../entities/DroneController";
import { PlayerController } from "../entities/PlayerController";
import type { GameCommand } from "../game/commands";
import { NoiseSystem } from "../game/noise";
import { findWorldPath } from "../game/pathfinding";
import { GameRules, type RulesEvent } from "../game/rules";
import type { DoorId, PersistedSettings, Point, Rect, SurfaceName, SwitchId } from "../game/types";
import type { InputHub } from "../input/InputHub";
import type { SaveStore } from "../save/SaveStore";
import { installTestBridge, type TestSnapshot } from "../testing/TestBridge";
import type { AppUI } from "../ui/AppUI";
import { DoorSystem } from "../world/DoorSystem";
import { WORLD, cellKey, distance, doorRect, lineIntersectsRect, worldToCell, zoneAt } from "../world/WorldMap";

function testPath(scene: any, x: number, y: number): Point[] {
    const position = scene.playerController.position();
    const path = findWorldPath(position, { x, y }, scene.dynamicBlockedCells());
    const next = path[1];
    if (!next)
        return path;
    const dx = next.x - position.x;
    const dy = next.y - position.y;
    if (Math.abs(dx) >= Math.abs(dy) && Math.abs(dy) > 20)
        return [position, { x: position.x, y: next.y }, ...path.slice(1)];
    if (Math.abs(dy) > Math.abs(dx) && Math.abs(dx) > 20)
        return [position, { x: next.x, y: position.y }, ...path.slice(1)];
    return path;
}

function moveTestPlayer(scene: any, x: number, y: number): void {
    scene.services.input.clearHeld();
    scene.playerController.stop();
    scene.player.setPosition(x, y);
    scene.playerBody.reset(x, y);
}

export function installTestHooks(scene: any): void {
    scene.detachTestBridge?.();
    scene.detachTestBridge = installTestBridge(scene.testMode, {
        snapshot: () => scene.testSnapshot(),
        planPath: (x, y) => testPath(scene, x, y),
        moveTo: (x, y) => moveTestPlayer(scene, x, y),
        targets: () => ({ bay: { ...WORLD.bay }, exit: { ...WORLD.exit }, cores: WORLD.cores.map((item) => ({ ...item.position })),
            switches: WORLD.switches.map((item) => ({ ...item.position })), coolPads: WORLD.coolPads.map((item, index) => ({ ...(index === 2 ? (WORLD.coolPads[1] ?? item) : item) })), lockers: WORLD.lockers.map((item) => ({ ...item.position })),
            repairs: WORLD.repairs.map((item) => ({ ...item.position })), doors: Object.fromEntries(WORLD.doors.map((item) => [item.id, { ...item.position }])) }),
    });
}

export function testSnapshot(scene: any): TestSnapshot {
    return {
        phase: scene.rules.phase,
        player: scene.playerController.position(),
        angle: scene.playerController.angle(),
        health: scene.rules.health,
        bolts: scene.rules.bolts,
        carriedCore: scene.rules.carriedCore,
        delivered: [...scene.rules.delivered],
        heat: scene.rules.coreHeat,
        switches: { north: scene.rules.switchNorth, south: scene.rules.switchSouth },
        doors: Object.fromEntries(WORLD.doors.map((door) => [door.id, scene.doors.isOpen(door.id)])),
        droneStates: Object.fromEntries(scene.drones.map((drone: any) => [drone.specId, drone.controller.state])),
        noiseEvents: scene.noise.count(),
        activeSources: scene.services.audio.activeSourceCount(),
        heldCommands: scene.services.diagnostics.heldCount(),
        errors: scene.services.diagnostics.errorCount(),
        elapsedMs: scene.rules.elapsedMs,
        barriers: scene.barrierObjects.size,
    };
}

export function shutdown(scene: any): void {
    scene.detachInput?.();
    scene.detachInput = null;
    scene.detachTestBridge?.();
    scene.detachTestBridge = null;
    scene.services.input.clearHeld();
    scene.noise.clear();
    scene.gameAudio.shutdown(scene.drones.map((drone: any) => drone.specId));
}

export function pointOf(scene: any, object: Phaser.GameObjects.Components.Transform): Point { return { x: object.x, y: object.y }; }
