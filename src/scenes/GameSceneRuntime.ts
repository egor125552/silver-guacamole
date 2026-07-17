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

import { surfaceNoise } from "./GameSceneConstants";

function isDeterministicRuleScenario(scene: any): boolean {
    return scene.testMode && (scene.testScenario === "playthrough" || scene.testScenario === "overheat");
}

export function updatePlayerNoise(scene: any, delta: number, position: Point, moving: boolean, sprinting: boolean): void {
    scene.lastStepNoiseMs += delta;
    if (!moving)
        return;
    const interval = sprinting ? 250 : 430;
    if (scene.lastStepNoiseMs < interval)
        return;
    scene.lastStepNoiseMs = 0;
    const zone = WORLD.zones.find((item) => item.name === zoneAt(position));
    const radius = (zone ? surfaceNoise[zone.surface] : 330) * (sprinting ? 1.42 : 1) * (scene.rules.carriedCore ? 1.14 : 1);
    scene.noise.emit(sprinting ? "sprint" : "step", position, radius, 950, sprinting ? 1.2 : 0.72);
}

export function updateDrones(scene: any, delta: number, playerPoint: Point): void {
    if (isDeterministicRuleScenario(scene)) {
        for (const drone of scene.drones)
            drone.body.setVelocity(0, 0);
        return;
    }
    const blocked = scene.dynamicBlockedCells();
    const blockers = [...WORLD.walls, ...scene.dynamicRects()];
    for (const drone of scene.drones) {
        if (scene.rules.phase === "lockdown")
            drone.controller.activate();
        const position = scene.pointOf(drone.object);
        const canSee = distance(position, playerPoint) < drone.controller.spec.sightRadius && !blockers.some((rect) => lineIntersectsRect(position, playerPoint, rect));
        const heard = scene.noise.strongestFor(position, drone.controller.spec.hearingRadius);
        const decision = drone.controller.update(delta, position, playerPoint, blocked, canSee, heard, scene.rules.phase === "lockdown");
        drone.body.setVelocity(decision.velocity.x, decision.velocity.y);
        drone.object.rotation = Math.atan2(decision.velocity.y, decision.velocity.x);
        drone.object.setAlpha(drone.controller.state === "dormant" ? 0.18 : drone.controller.state === "stunned" ? 0.45 : 1);
        if (decision.warning)
            scene.services.announcements.announce("Дрон сближается. Слышен предупредительный импульс.", "danger");
        if (distance(position, playerPoint) < 38 && drone.controller.state !== "stunned" && drone.controller.state !== "dormant") {
            scene.rules.damage(1, `${drone.controller.spec.kind === "listener" ? "поисковый" : "охранный"} дрон`);
            drone.controller.stun(900);
            void scene.services.audio.oneShot("impact", playerPoint, { category: "danger", priority: 96, volume: 0.95 });
        }
    }
}

export function updateTrolley(scene: any, delta: number, playerPoint: Point): void {
    const route = scene.rules.switchNorth || scene.rules.switchSouth ? WORLD.trolleyRouteB : WORLD.trolleyRouteA;
    const current = scene.pointOf(scene.trolley);
    const target = route[scene.trolleyIndex] ?? route[0] ?? current;
    if (distance(current, target) < 34)
        scene.trolleyIndex = (scene.trolleyIndex + 1) % route.length;
    const next = route[scene.trolleyIndex] ?? target;
    const angle = Math.atan2(next.y - current.y, next.x - current.x);
    const speed = scene.testScenario === "trolley" ? 0 : 122 * (scene.rules.phase === "lockdown" ? 1.22 : 1);
    scene.trolleyBody.setVelocity(Math.cos(angle) * speed, Math.sin(angle) * speed);
    scene.trolley.rotation = angle;
    scene.lastTrolleyNoiseMs += delta;
    if (scene.lastTrolleyNoiseMs >= 900) {
        scene.lastTrolleyNoiseMs = 0;
        scene.noise.emit("trolley", current, 580, 1000, 1.05);
    }
    for (const barrier of WORLD.barriers) {
        const object = scene.barrierObjects.get(barrier.id);
        if (!object || distance(current, scene.pointOf(object)) > 58 || (!scene.rules.switchNorth && !scene.rules.switchSouth))
            continue;
        object.destroy();
        scene.barrierObjects.delete(barrier.id);
        scene.noise.emit("impact", current, 720, 2200, 1.4);
        void scene.services.audio.oneShot("gate", current, { category: "danger", priority: 82, volume: 0.9 });
        scene.refreshDynamicBlockers();
    }
    if (!isDeterministicRuleScenario(scene) && distance(current, playerPoint) < 48)
        scene.rules.damage(1, "автономная грузовая тележка");
    for (const drone of scene.drones)
        if (distance(current, scene.pointOf(drone.object)) < 48)
            drone.controller.stun(3600);
}

export function updateBolts(scene: any, delta: number): void {
    for (let index = scene.bolts.length - 1; index >= 0; index -= 1) {
        const bolt = scene.bolts[index];
        if (!bolt)
            continue;
        bolt.lifeMs -= delta;
        if (bolt.lifeMs <= 0) {
            bolt.object.destroy();
            scene.bolts.splice(index, 1);
        }
    }
}
