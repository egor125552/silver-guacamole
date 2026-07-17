import Phaser from "phaser";
import { InputHub } from "../input/InputHub";
import type { Point } from "../game/types";

const wrapAngle = (angle: number): number => Math.atan2(Math.sin(angle), Math.cos(angle));

export class PlayerController {
  private baseSpeed = 124;
  private testMultiplier = 1;
  constructor(
    private readonly object: Phaser.GameObjects.Rectangle,
    private readonly body: Phaser.Physics.Arcade.Body,
    private readonly input: InputHub,
  ) {}

  setTestMultiplier(value: number): void { this.testMultiplier = Math.max(1, value); }
  turnBy(amount: number): void { this.object.rotation = wrapAngle(this.object.rotation + amount); }
  stop(): void { this.body.setVelocity(0, 0); }
  position(): Point { return { x: this.object.x, y: this.object.y }; }
  angle(): number { return this.object.rotation; }

  update(deltaMs: number, carrying: boolean): { moving: boolean; sprinting: boolean; speed: number } {
    const turn = this.input.currentTurn();
    if (turn !== 0) this.object.rotation = wrapAngle(this.object.rotation + turn * deltaMs * 0.0026);
    const amount = this.input.currentMove();
    if (amount === 0) { this.body.setVelocity(0, 0); return { moving: false, sprinting: false, speed: 0 }; }
    const sprinting = Math.abs(amount) > 1.15;
    const carryFactor = carrying ? 0.82 : 1;
    const speed = this.baseSpeed * Math.abs(amount) * carryFactor * this.testMultiplier;
    const direction = amount >= 0 ? 1 : -1;
    this.body.setVelocity(Math.cos(this.object.rotation) * speed * direction, Math.sin(this.object.rotation) * speed * direction);
    return { moving: true, sprinting, speed };
  }
}
