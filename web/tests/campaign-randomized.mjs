import {EventBus} from '../js/core/events.js';
import {Settings} from '../js/core/constants.js';
import {hasLineOfSight,pointInRect} from '../js/core/math.js';
import {CampaignStages} from '../js/plugins/campaign/state.js';
const base=new URL('../js/plugins/',import.meta.url);const names=['world/level.js','player/state.js','enemy/loadouts.js','enemy/population.js','campaign/state.js','campaign/weapon-unlocks.js','campaign/director.js'];
class Game{constructor(){this.bus=new EventBus();this.settings={...Settings};this.time=0;this.state={mode:'playing',player:null,enemies:[],walls:[],shadowZones:[],room:'generic'};this.audio={play:()=>({})};}on(n,f){return this.bus.on(n,f)}emit(n,p){this.bus.emit(n,p)}hasLOS(a,b){return hasLineOfSight(a,b,this.state.walls)}status(t){this.lastStatus=t}}
const ok=(c,m)=>{if(!c)throw new Error(m)};
for(let run=0;run<60;run++){
 const g=new Game();for(const name of names){const m=await import(new URL(name,base));await m.default.install(g);}g.emit('reset');
 for(const stage of CampaignStages){ok(!g.state.walls.some(w=>pointInRect(stage.target,w)),`objective ${stage.id} inside wall on run ${run}`);}
 const roles=new Set(g.state.enemies.map(e=>e.role));ok(roles.has('sentry')&&roles.has('investigator')&&roles.has('charger'),`missing behavior diversity on run ${run}`);
 const steps=new Set(g.state.enemies.map(e=>e.nextStep.toFixed(3)));ok(steps.size>8,`enemy step phases synchronized on run ${run}`);
 ok(g.state.enemies.length>=16,'campaign population unexpectedly empty');
}
console.log('Campaign randomized safety passed: 60 generated worlds');
