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

export function handleCommand(scene: any, command: GameCommand): void {
    if (command.type === "turn" && Number.isFinite(command.amount))
        scene.playerController.turnBy(command.amount);
    else if (command.type === "interact")
        scene.interact();
    else if (command.type === "special")
        scene.throwBolt();
    else if (command.type === "status")
        scene.announceStatus();
    else if (command.type === "instruction")
        scene.announceInstruction();
    else if (command.type === "pause")
        scene.togglePause();
    else if (command.type === "emergency-stop")
        void scene.emergencyStop();
}

export function interact(scene: any): void {
    if (scene.rules.phase === "paused" || scene.rules.phase === "won" || scene.rules.phase === "lost")
        return;
    const player = scene.playerController.position();
    if (distance(player, WORLD.bay) < 72 && scene.rules.carriedCore) {
        scene.rules.deliverCore();
        void scene.services.audio.oneShot("pickup", WORLD.bay, { category: "ui", priority: 88, volume: 0.92 });
        scene.flushRuleEvents();
        return;
    }
    if (distance(player, WORLD.exit) < 74 && scene.rules.phase === "lockdown") {
        if (scene.rules.canExit())
            scene.rules.win();
        else
            scene.services.announcements.announce("Диспетчерская обесточена. Нужны оба силовых переключателя.");
        scene.flushRuleEvents();
        return;
    }
    const nearbyDoor = WORLD.doors.filter((door) => distance(player, door.position) < 68).sort((a, b) => distance(player, a.position) - distance(player, b.position))[0];
    if (nearbyDoor) {
        if (nearbyDoor.requiresSwitch)
            scene.services.announcements.announce(`${nearbyDoor.name} управляется силовым переключателем.`);
        else {
            const open = scene.doors.toggle(nearbyDoor.id);
            scene.applyDoorBody(nearbyDoor.id);
            scene.refreshDynamicBlockers();
            scene.noise.emit("door", nearbyDoor.position, 520, 2200, 1.1);
            void scene.services.audio.oneShot(open ? "door-open" : "door-close", nearbyDoor.position, { category: "mechanisms", priority: 70, volume: 0.82 });
            scene.services.announcements.announce(`${nearbyDoor.name} ${open ? "открыта" : "закрыта"}.`);
        }
        return;
    }
    for (const spec of WORLD.switches) {
        if (distance(player, spec.position) >= 68)
            continue;
        const value = scene.rules.toggleSwitch(spec.id);
        scene.switchObjects.get(spec.id)?.setFillStyle(value ? 0x43a65d : 0xb16e2e);
        for (const id of scene.doors.syncSwitch(spec.id, value))
            scene.applyDoorBody(id);
        scene.refreshDynamicBlockers();
        scene.noise.emit("impact", spec.position, 620, 2200, 1.2);
        void scene.services.audio.oneShot("switch", spec.position, { category: "mechanisms", priority: 76, volume: 0.86 });
        scene.flushRuleEvents();
        return;
    }
    for (const spec of WORLD.lockers) {
        if (distance(player, spec.position) < 68 && scene.rules.openLocker(spec.id, spec.reward)) {
            scene.lockerObjects.get(spec.id)?.setAlpha(0.32);
            scene.noise.emit("door", spec.position, 480, 1800, 0.95);
            void scene.services.audio.oneShot("door-open", spec.position, { category: "mechanisms", priority: 64, volume: 0.78 });
            scene.flushRuleEvents();
            return;
        }
    }
    for (const spec of WORLD.repairs) {
        if (distance(player, spec.position) < 68 && scene.rules.repair(spec.id)) {
            scene.repairObjects.get(spec.id)?.setAlpha(0.3);
            void scene.services.audio.oneShot("cooling-loop", spec.position, { category: "mechanisms", priority: 62, volume: 0.65 });
            scene.flushRuleEvents();
            return;
        }
    }
    const nearest = scene.cores.nearestAvailable(player, 66);
    if (nearest && scene.rules.pickCore(nearest.id)) {
        scene.cores.carry(nearest.id);
        scene.gameAudio.pickCore(nearest.id);
        void scene.services.audio.oneShot("pickup", nearest.position, { category: "mechanisms", priority: 70, volume: 0.82 });
        scene.flushRuleEvents();
        return;
    }
    if (scene.rules.carriedCore) {
        const dropped = scene.rules.dropCore(player);
        if (dropped)
            scene.cores.drop(dropped, player);
        void scene.services.audio.oneShot("drop", player, { category: "mechanisms", priority: 64, volume: 0.78 });
        scene.flushRuleEvents();
        return;
    }
    scene.services.announcements.announce("Рядом нечего использовать.");
}

export function throwBolt(scene: any): void {
    const player = scene.playerController.position();
    if (!scene.rules.useBolt()) {
        scene.services.announcements.announce("Болтов больше нет.");
        return;
    }
    const position = {
        x: Phaser.Math.Clamp(player.x + Math.cos(scene.player.rotation) * 190, 42, WORLD.width - 42),
        y: Phaser.Math.Clamp(player.y + Math.sin(scene.player.rotation) * 190, 42, WORLD.height - 42),
    };
    const object = scene.add.circle(position.x, position.y, 7, 0xe3e6e8, 1);
    scene.bolts.push({ object, lifeMs: 5200 });
    scene.noise.emit("bolt", position, 820, 5200, 1.65);
    void scene.services.audio.oneShot("bolt", position, { category: "danger", priority: 88, volume: 0.94 });
    scene.flushRuleEvents();
}
