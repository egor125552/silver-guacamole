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
import { drawZones as drawZonesImpl, createBlockers as createBlockersImpl, addStaticBlocker as addStaticBlockerImpl, createObjects as createObjectsImpl, createPlayer as createPlayerImpl, createDrones as createDronesImpl, createTrolley as createTrolleyImpl, configureCollisions as configureCollisionsImpl, dynamicRects as dynamicRectsImpl, dynamicBlockedCells as dynamicBlockedCellsImpl, refreshDynamicBlockers as refreshDynamicBlockersImpl, applyDoorBody as applyDoorBodyImpl } from "./GameSceneWorld";
import { updatePlayerNoise as updatePlayerNoiseImpl, updateDrones as updateDronesImpl, updateTrolley as updateTrolleyImpl, updateBolts as updateBoltsImpl } from "./GameSceneRuntime";
import { handleCommand as handleCommandImpl, interact as interactImpl, throwBolt as throwBoltImpl } from "./GameSceneInteraction";
import { flushRuleEvents as flushRuleEventsImpl, handleRuleEvent as handleRuleEventImpl, announceStatus as announceStatusImpl, announceInstruction as announceInstructionImpl, togglePause as togglePauseImpl, emergencyStop as emergencyStopImpl, updateHud as updateHudImpl } from "./GameSceneEvents";
import { installTestHooks as installTestHooksImpl, testSnapshot as testSnapshotImpl, shutdown as shutdownImpl, pointOf as pointOfImpl } from "./GameSceneTesting";

interface GameServices {
  input: InputHub;
  audio: AajaAudioAdapter;
  audioScene: AudioScene;
  announcements: AnnouncementQueue;
  diagnostics: GameDiagnostics;
  ui: AppUI;
  settings: () => PersistedSettings;
  save: SaveStore;
  onRestart: () => void;
}

interface DroneRuntime { specId: string; object: Phaser.GameObjects.Arc; body: Phaser.Physics.Arcade.Body; controller: DroneController; }

interface BoltRuntime { object: Phaser.GameObjects.Arc; lifeMs: number; }

export class GameScene extends Phaser.Scene {
  private readonly rules = new GameRules(WORLD.cores.length);
  private readonly doors = new DoorSystem(WORLD.doors);
  private readonly noise = new NoiseSystem();
  private readonly cores = new CoreManager();
  private player!: Phaser.GameObjects.Rectangle;
  private playerBody!: Phaser.Physics.Arcade.Body;
  private playerController!: PlayerController;
  private blockers!: Phaser.Physics.Arcade.StaticGroup;
  private readonly doorObjects = new Map<DoorId, Phaser.GameObjects.Rectangle>();
  private readonly barrierObjects = new Map<string, Phaser.GameObjects.Rectangle>();
  private readonly switchObjects = new Map<SwitchId, Phaser.GameObjects.Arc>();
  private readonly lockerObjects = new Map<string, Phaser.GameObjects.Rectangle>();
  private readonly repairObjects = new Map<string, Phaser.GameObjects.Arc>();
  private readonly drones: DroneRuntime[] = [];
  private trolley!: Phaser.GameObjects.Rectangle;
  private trolleyBody!: Phaser.Physics.Arcade.Body;
  private trolleyIndex = 0;
  private bolts: BoltRuntime[] = [];
  private detachInput: (() => void) | null = null;
  private detachTestBridge: (() => void) | null = null;
  private lastStepNoiseMs = 0;
  private lastTrolleyNoiseMs = 0;
  private readonly gameAudio: GameAudioController;
  private testMode = false;
  private testScenario = "";
  private timeScale = 1;

  constructor(private readonly services: GameServices) { super({ key: "GameScene" }); this.gameAudio = new GameAudioController(services.audio, services.audioScene); }

  create(): void {
    this.testMode = new URLSearchParams(location.search).get("testMode") === "1";
    this.testScenario = this.testMode ? (new URLSearchParams(location.search).get("scenario") ?? "") : "";
    this.timeScale = this.testMode ? Phaser.Math.Clamp(Number(new URLSearchParams(location.search).get("timeScale") ?? 1) || 1, 1, 30) : 1;
    this.rules.reset();
    this.doors.reset();
    this.noise.clear();
    this.cores.reset();
    this.drones.length = 0;
    this.bolts = [];
    this.doorObjects.clear();
    this.barrierObjects.clear();
    this.switchObjects.clear();
    this.lockerObjects.clear();
    this.repairObjects.clear();
    this.services.audioScene.reset();

    this.physics.world.setBounds(0, 0, WORLD.width, WORLD.height);
    this.cameras.main.setBounds(0, 0, WORLD.width, WORLD.height);
    this.drawZones();
    this.createBlockers();
    this.createObjects();
    this.createPlayer();
    this.createDrones();
    this.createTrolley();
    this.configureCollisions();
    this.refreshDynamicBlockers();

    this.detachInput = this.services.input.subscribe((command) => this.handleCommand(command));
    this.events.once(Phaser.Scenes.Events.SHUTDOWN, () => this.shutdown());
    this.rules.start();
    this.flushRuleEvents();
    this.announceInstruction();
    this.installTestHooks();
    void this.gameAudio.start(
      this.pointOf(this.trolley),
      this.cores.values().map((core) => ({ id: core.id, position: core.position })),
      this.drones.map((drone) => ({ id: drone.specId, kind: drone.controller.spec.kind, position: this.pointOf(drone.object), state: drone.controller.state })),
    );
  }

  override update(_time: number, rawDelta: number): void {
    if (!this.playerBody || this.rules.phase === "paused" || this.rules.phase === "won" || this.rules.phase === "lost") return;
    const delta = Math.min(50, rawDelta) * this.timeScale;
    const playerPoint = this.playerController.position();
    const movement = this.playerController.update(rawDelta, Boolean(this.rules.carriedCore));
    const onCooling = WORLD.coolPads.some((point) => distance(playerPoint, point) < 68);
    this.rules.update(delta, onCooling, playerPoint);
    this.noise.update();
    this.updatePlayerNoise(delta, playerPoint, movement.moving, movement.sprinting);
    this.updateDrones(delta, playerPoint);
    this.updateTrolley(delta, playerPoint);
    this.updateBolts(delta);
    this.gameAudio.update({
      deltaMs: delta, player: playerPoint, angle: this.player.rotation, moving: movement.moving, sprinting: movement.sprinting,
      carriedCore: Boolean(this.rules.carriedCore), coreHeat: this.rules.coreHeat, trolley: this.pointOf(this.trolley),
      drones: this.drones.map((drone) => ({ id: drone.specId, kind: drone.controller.spec.kind, position: this.pointOf(drone.object), state: drone.controller.state })),
    });
    this.flushRuleEvents();
    this.updateHud();
  }

    private drawZones(): void { return drawZonesImpl(this); }

    private createBlockers(): void { return createBlockersImpl(this); }

    private addStaticBlocker(rect: Rect, colour: number, alpha: number): void { return addStaticBlockerImpl(this, rect, colour, alpha); }

    private createObjects(): void { return createObjectsImpl(this); }

    private createPlayer(): void { return createPlayerImpl(this); }

    private createDrones(): void { return createDronesImpl(this); }

    private createTrolley(): void { return createTrolleyImpl(this); }

    private configureCollisions(): void { return configureCollisionsImpl(this); }

    private dynamicRects(): Rect[] { return dynamicRectsImpl(this); }

    private dynamicBlockedCells(): Set<string> { return dynamicBlockedCellsImpl(this); }

    private refreshDynamicBlockers(): void { return refreshDynamicBlockersImpl(this); }

    private applyDoorBody(id: DoorId): void { return applyDoorBodyImpl(this, id); }

    private updatePlayerNoise(delta: number, position: Point, moving: boolean, sprinting: boolean): void { return updatePlayerNoiseImpl(this, delta, position, moving, sprinting); }

    private updateDrones(delta: number, playerPoint: Point): void { return updateDronesImpl(this, delta, playerPoint); }

    private updateTrolley(delta: number, playerPoint: Point): void { return updateTrolleyImpl(this, delta, playerPoint); }

    private updateBolts(delta: number): void { return updateBoltsImpl(this, delta); }

    private handleCommand(command: GameCommand): void { return handleCommandImpl(this, command); }

    private interact(): void { return interactImpl(this); }

    private throwBolt(): void { return throwBoltImpl(this); }

    private flushRuleEvents(): void { return flushRuleEventsImpl(this); }

    private handleRuleEvent(event: RulesEvent): void { return handleRuleEventImpl(this, event); }

    private announceStatus(): void { return announceStatusImpl(this); }

    private announceInstruction(): void { return announceInstructionImpl(this); }

    private togglePause(): void { return togglePauseImpl(this); }

    private async emergencyStop(): Promise<void> { return emergencyStopImpl(this); }

    private updateHud(): void { return updateHudImpl(this); }

    private installTestHooks(): void { return installTestHooksImpl(this); }

    private testSnapshot(): TestSnapshot { return testSnapshotImpl(this); }

    private shutdown(): void { return shutdownImpl(this); }

    private pointOf(object: Phaser.GameObjects.Components.Transform): Point { return pointOfImpl(this, object); }
}
