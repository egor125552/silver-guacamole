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

import { coreColours, droneColours } from "./GameSceneConstants";
export function drawZones(scene: any): void {
    const colours: Record<string, number> = { yard: 0x17212b, hangar: 0x26313c, corridor: 0x202936, shaft: 0x242b34, cooling: 0x17333a, machine: 0x302a2a };
    for (const zone of WORLD.zones) {
        scene.add.rectangle(zone.rect.x + zone.rect.width / 2, zone.rect.y + zone.rect.height / 2, zone.rect.width, zone.rect.height, colours[zone.name] ?? 0x202a34, 1).setDepth(-10);
        scene.add.text(zone.rect.x + 16, zone.rect.y + 12, zone.label, { color: "#a9bdd0", fontSize: "18px" }).setDepth(-9);
    }
}

export function createBlockers(scene: any): void {
    scene.blockers = scene.physics.add.staticGroup();
    for (const rect of WORLD.walls)
        scene.addStaticBlocker(rect, 0x43505d, 0.72);
    for (const spec of WORLD.doors) {
        const rect = doorRect(spec);
        const object = scene.add.rectangle(spec.position.x, spec.position.y, rect.width, rect.height, spec.initiallyOpen ? 0x2e8b57 : 0xb06f2d, spec.initiallyOpen ? 0.25 : 0.95);
        scene.physics.add.existing(object, true);
        scene.blockers.add(object);
        scene.doorObjects.set(spec.id, object);
        scene.applyDoorBody(spec.id);
    }
    for (const barrier of WORLD.barriers) {
        const object = scene.add.rectangle(barrier.rect.x + barrier.rect.width / 2, barrier.rect.y + barrier.rect.height / 2, barrier.rect.width, barrier.rect.height, 0x8d6641, 0.96);
        scene.physics.add.existing(object, true);
        scene.blockers.add(object);
        scene.barrierObjects.set(barrier.id, object);
    }
}

export function addStaticBlocker(scene: any, rect: Rect, colour: number, alpha: number): void {
    const object = scene.add.rectangle(rect.x + rect.width / 2, rect.y + rect.height / 2, rect.width, rect.height, colour, alpha);
    scene.physics.add.existing(object, true);
    scene.blockers.add(object);
}

export function createObjects(scene: any): void {
    scene.add.circle(WORLD.bay.x, WORLD.bay.y, 42, 0x4ca6a8, 0.9);
    scene.add.text(WORLD.bay.x - 42, WORLD.bay.y - 72, "Лифт", { color: "#d7ffff", fontSize: "18px" });
    scene.add.rectangle(WORLD.exit.x, WORLD.exit.y, 62, 62, 0x2e8b57, 0.85);
    scene.add.text(WORLD.exit.x - 64, WORLD.exit.y - 54, "Диспетчерская", { color: "#d9ffe6", fontSize: "16px" });
    for (const spec of WORLD.cores) {
        const object = scene.add.circle(spec.position.x, spec.position.y, 18, coreColours[spec.id] ?? 0xffffff, 1);
        scene.physics.add.existing(object, true);
        scene.cores.add(spec, object);
    }
    for (const point of WORLD.coolPads)
        scene.add.circle(point.x, point.y, 32, 0x4cb8c4, 0.44).setStrokeStyle(3, 0x91f1ff, 0.85);
    for (const spec of WORLD.switches) {
        const object = scene.add.circle(spec.position.x, spec.position.y, 22, 0xb16e2e, 1);
        scene.switchObjects.set(spec.id, object);
    }
    for (const spec of WORLD.lockers) {
        const object = scene.add.rectangle(spec.position.x, spec.position.y, 42, 42, 0x6d7f91, 1);
        scene.lockerObjects.set(spec.id, object);
    }
    for (const spec of WORLD.repairs) {
        const object = scene.add.circle(spec.position.x, spec.position.y, 22, 0x54a96b, 0.9);
        scene.repairObjects.set(spec.id, object);
    }
}

export function createPlayer(scene: any): void {
    let start = WORLD.start;
    if (scene.testScenario === "drone")
        start = { ...WORLD.droneSpecs[0]?.route[0] ?? WORLD.start };
    else if (scene.testScenario === "trolley")
        start = { ...WORLD.trolleyRouteA[0] ?? WORLD.start };
    else if (scene.testScenario === "overheat")
        start = { ...WORLD.cores[0]?.position ?? WORLD.start };
    scene.player = scene.add.rectangle(start.x, start.y, 30, 30, 0xe8f0f2, 1);
    scene.physics.add.existing(scene.player);
    scene.playerBody = scene.player.body as Phaser.Physics.Arcade.Body;
    scene.playerBody.setCollideWorldBounds(true).setDrag(420, 420).setMaxVelocity(520, 520);
    scene.playerController = new PlayerController(scene.player, scene.playerBody, scene.services.input);
    if (scene.testMode)
        scene.playerController.setTestMultiplier(3);
    scene.cameras.main.startFollow(scene.player, true, 0.13, 0.13);
}

export function createDrones(scene: any): void {
    for (const spec of WORLD.droneSpecs) {
        const route = spec.id === "hangar-sentinel"
            ? [...spec.route.slice(2), ...spec.route.slice(0, 2)]
            : spec.route;
        const runtimeSpec = route === spec.route ? spec : { ...spec, route };
        const start = runtimeSpec.route[0] ?? WORLD.start;
        const object = scene.add.circle(start.x, start.y, runtimeSpec.kind === "interceptor" ? 20 : 17, droneColours[runtimeSpec.kind], runtimeSpec.activateOnLockdown ? 0.18 : 1);
        scene.physics.add.existing(object);
        const body = object.body as Phaser.Physics.Arcade.Body;
        body.setCollideWorldBounds(true).setMaxVelocity(260, 260);
        const controller = new DroneController(runtimeSpec);
        if (scene.testScenario === "overheat")
            controller.stun(600_000);
        scene.drones.push({ specId: runtimeSpec.id, object, body, controller });
    }
}

export function createTrolley(scene: any): void {
    const start = WORLD.trolleyRouteA[0] ?? WORLD.start;
    scene.trolley = scene.add.rectangle(start.x, start.y, 52, 34, 0xe0b04b, 1);
    scene.physics.add.existing(scene.trolley);
    scene.trolleyBody = scene.trolley.body as Phaser.Physics.Arcade.Body;
    scene.trolleyBody.setImmovable(true).setMaxVelocity(190, 190);
}

export function configureCollisions(scene: any): void {
    scene.physics.add.collider(scene.player, scene.blockers);
    for (const drone of scene.drones)
        scene.physics.add.collider(drone.object, scene.blockers, () => drone.controller.stun(180));
}

export function dynamicRects(scene: any): Rect[] {
    const doors = WORLD.doors.filter((door) => !scene.doors.isOpen(door.id)).map(doorRect);
    const barriers = WORLD.barriers.filter((barrier) => scene.barrierObjects.has(barrier.id)).map((barrier) => barrier.rect);
    return [...doors, ...barriers];
}

export function dynamicBlockedCells(scene: any): Set<string> {
    const result = new Set(WORLD.blockedCells);
    for (const key of scene.doors.blockingCells())
        result.add(key);
    for (const barrier of WORLD.barriers) {
        if (!scene.barrierObjects.has(barrier.id))
            continue;
        const left = Math.floor(barrier.rect.x / WORLD.tileSize);
        const right = Math.floor((barrier.rect.x + barrier.rect.width - 1) / WORLD.tileSize);
        const top = Math.floor(barrier.rect.y / WORLD.tileSize);
        const bottom = Math.floor((barrier.rect.y + barrier.rect.height - 1) / WORLD.tileSize);
        for (let row = top; row <= bottom; row += 1)
            for (let col = left; col <= right; col += 1)
                result.add(cellKey({ col, row }));
    }
    return result;
}

export function refreshDynamicBlockers(scene: any): void {
    for (const door of WORLD.doors)
        scene.applyDoorBody(door.id);
    scene.services.audioScene.setDynamicBlockers(scene.dynamicRects());
}

export function applyDoorBody(scene: any, id: DoorId): void {
    const object = scene.doorObjects.get(id);
    if (!object?.body)
        return;
    const body = object.body as Phaser.Physics.Arcade.StaticBody;
    const open = scene.doors.isOpen(id);
    body.enable = !open;
    object.setFillStyle(open ? 0x2e8b57 : 0xb06f2d, open ? 0.25 : 0.95);
    if (!open)
        body.updateFromGameObject();
}
