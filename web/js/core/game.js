import {EventBus} from "./events.js";
import {PluginHost} from "./plugin-host.js";
import {InputState} from "./input.js";
import {Settings} from "./constants.js";
import {hasLineOfSight} from "./math.js";
import {plugins} from "../plugins/registry.js";

export class Game {
  constructor(statusNode) {
    this.bus = new EventBus();
    this.plugins = new PluginHost(this);
    this.input = new InputState(this.bus);
    this.settings = {...Settings};
    this.statusNode = statusNode;
    this.audio = null;
    this.running = false;
    this.lastFrame = 0;
    this.time = 0;
    this.state = {mode:"playing", player:null, enemies:[], walls:[], shadowZones:[], room:"generic"};
  }
  on(name, fn) { return this.bus.on(name, fn); }
  emit(name, payload) { this.bus.emit(name, payload); }
  status(text) { if (this.statusNode) this.statusNode.textContent=text; this.emit("status",text); }
  hasLOS(from,to) { return hasLineOfSight(from,to,this.state.walls); }
  async start() {
    if (this.running) return;
    await this.plugins.load(plugins);
    this.input.attach();
    this.running=true;
    this.emit("start");
    this.emit("reset");
    this.lastFrame=performance.now();
    requestAnimationFrame(t=>this.frame(t));
  }
  frame(now) {
    if (!this.running) return;
    const dt=Math.min((now-this.lastFrame)/1000,.1);
    this.lastFrame=now; this.time+=dt;
    this.emit("update",{dt,time:this.time});
    requestAnimationFrame(t=>this.frame(t));
  }
  reset() { this.time=0; this.state.mode="playing"; this.emit("reset"); }
  stop() { this.running=false; this.input.detach(); this.emit("stop"); this.status("Игра остановлена."); }
}
