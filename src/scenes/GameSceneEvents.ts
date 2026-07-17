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

export function flushRuleEvents(scene: any): void {
    while (scene.rules.events.length > 0) {
        const event = scene.rules.events.shift();
        if (event)
            scene.handleRuleEvent(event);
    }
}

export function handleRuleEvent(scene: any, event: RulesEvent): void {
    if (event.type === "announce")
        scene.services.announcements.announce(event.text, event.priority ?? "normal");
    else if (event.type === "core-dropped") {
        scene.cores.drop(event.core, event.position);
        scene.gameAudio.dropCore(event.core, event.position);
    }
    else if (event.type === "core-delivered") {
        scene.cores.deliver(event.core);
        scene.gameAudio.deliverCore(event.core);
    }
    else if (event.type === "lockdown") {
        for (const id of scene.doors.closePoweredDoors())
            scene.applyDoorBody(id);
        scene.switchObjects.get("north")?.setFillStyle(0xb16e2e);
        scene.switchObjects.get("south")?.setFillStyle(0xb16e2e);
        scene.refreshDynamicBlockers();
        scene.noise.emit("alarm", WORLD.bay, 1200, 8000, 2);
        void scene.services.audio.oneShot("alarm", WORLD.bay, { category: "danger", priority: 100, volume: 1 });
    }
    else if (event.type === "win") {
        const save = scene.services.save.recordWin(event.score);
        scene.playerController.stop();
        void scene.services.audio.stopLoop("player-footsteps");
        scene.services.announcements.announce(`Смена завершена. Результат ${event.score}. Лучший результат ${save.bestScore}.`);
        scene.services.ui.showResult("Смена завершена", `Результат: ${event.score}. Лучший: ${save.bestScore}. Четыре ядра доставлены, оба силовых контура восстановлены.`);
        void scene.services.audio.oneShot("success", WORLD.exit, { category: "ui", priority: 100, volume: 1 });
    }
    else if (event.type === "lose") {
        scene.playerController.stop();
        void scene.services.audio.stopLoop("player-footsteps");
        scene.services.announcements.announce(event.reason, "danger");
        scene.services.ui.showResult("Смена провалена", `${event.reason} Можно сразу перезапустить игру.`);
        void scene.services.audio.oneShot("failure", scene.playerController.position(), { category: "danger", priority: 100, volume: 1 });
    }
}

export function announceStatus(scene: any): void {
    const point = scene.playerController.position();
    const zone = WORLD.zones.find((item) => item.name === zoneAt(point));
    const carried = scene.rules.carriedCore ? `Несёшь ${scene.rules.carriedCore}, нагрев ${Math.round(scene.rules.coreHeat)} процентов.` : "Ядра в руках нет.";
    const lockdown = scene.rules.phase === "lockdown" ? `До закрытия шлюза ${Math.ceil(scene.rules.lockdownRemainingMs / 1000)} секунд.` : "";
    scene.services.announcements.announce(`Зона: ${zone?.label ?? "неизвестная"}. Прочность ${scene.rules.health} из ${scene.rules.maxHealth}. Болтов ${scene.rules.bolts}. Доставлено ${scene.rules.delivered.size} из ${scene.rules.requiredCores}. ${carried} Северный контур ${scene.rules.switchNorth ? "включён" : "выключен"}, южный ${scene.rules.switchSouth ? "включён" : "выключен"}. ${lockdown}`);
}

export function announceInstruction(scene: any): void {
    const verbosity = scene.services.settings().verbosity;
    const base = "Найди четыре звучащих энергоядра и доставь их к центральному лифту. Ядро нагревается в руках; охлаждающие площадки дают время. Шаги, двери и механизмы привлекают дронов, а брошенный болт отвлекает их.";
    const extra = verbosity === "minimal" ? "" : " Открывай обычные двери действием. Силовые двери зависят от северного и южного переключателей. Безопасные маршруты длиннее, короткие проходят рядом с патрулями и тележкой.";
    const detailed = verbosity === "detailed" ? " После четвёртого ядра начинается трёхминутная блокировка: силовые двери закрываются, появляется перехватчик, а для выхода нужны оба переключателя." : "";
    scene.services.announcements.announce(base + extra + detailed);
}

export function togglePause(scene: any): void {
    if (scene.rules.phase === "won" || scene.rules.phase === "lost")
        return;
    const paused = scene.rules.phase === "paused";
    scene.rules.pause(!paused);
    scene.physics.world.isPaused = !paused;
    if (paused) {
        scene.services.ui.hidePause();
        scene.services.announcements.announce("Игра продолжена.");
    }
    else {
        scene.services.input.clearHeld();
        scene.playerController.stop();
        scene.services.ui.showPause();
        scene.services.announcements.announce("Пауза.");
    }
    scene.flushRuleEvents();
}

export async function emergencyStop(scene: any): Promise<void> {
    scene.services.input.clearHeld();
    scene.playerController.stop();
    await scene.services.audio.emergencyStop();
    scene.services.ui.announce("Все игровые звуки остановлены. Следующая команда восстановит аудиоконтекст.", true);
}

export function updateHud(scene: any): void {
    const state = scene.rules.snapshot();
    scene.services.ui.updateHud(`Зона: ${zoneAt(scene.playerController.position())}. Прочность ${state.health}/${state.maxHealth}. Болты ${state.bolts}/${state.maxBolts}. Ядра ${state.delivered.size}/${state.requiredCores}. Нагрев ${Math.round(state.coreHeat)}%. Фаза: ${state.phase}.`);
}
