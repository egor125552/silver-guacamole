import {EventBus} from '../js/core/events.js';
import {Settings,Weapon} from '../js/core/constants.js';
import {hasLineOfSight} from '../js/core/math.js';

const base=new URL('../js/plugins/',import.meta.url);
const names=['world/level.js','player/state.js','enemy/loadouts.js','enemy/population.js','campaign/state.js','campaign/weapon-unlocks.js','campaign/director.js','enemy/damage.js','combat/weapons.js'];

class Game{
  constructor(){
    this.bus=new EventBus();this.settings={...Settings};this.time=0;
    this.state={mode:'playing',player:null,enemies:[],walls:[],shadowZones:[],room:'generic'};
    this.statuses=[];this.sounds=[];
    this.audio={play:(n,o={})=>{this.sounds.push([n,o]);return{};}};
  }
  on(n,f){return this.bus.on(n,f)}
  emit(n,p){this.bus.emit(n,p)}
  hasLOS(a,b){return hasLineOfSight(a,b,this.state.walls)}
  status(t){this.statuses.push(t);this.lastStatus=t}
}

const g=new Game();
for(const name of names){const m=await import(new URL(name,base));await m.default.install(g);}
g.emit('reset');

const ok=(c,m)=>{if(!c)throw new Error(m)};
const tick=seconds=>{g.time+=seconds;g.emit('update',{dt:Math.min(seconds,.1),time:g.time});};
const reach=()=>{const pos=g.campaignStage().target;g.state.player.position={...pos};tick(.1);};
const killObjective=()=>{const id=g.state.campaign.objectiveEnemyId,e=g.state.enemies.find(x=>x.id===id);ok(e&&e.alive,`missing objective enemy at stage ${g.state.campaign.stage}`);e.alive=false;e.health=0;e.deathTime=g.time;g.emit('enemyDied',e);};
const ranged=new Set([Weapon.PISTOL,Weapon.TASER,Weapon.AUTOMATIC,Weapon.SNIPER]);

ok(g.state.campaign.stage===0,'campaign starts at stage 0');
ok(g.state.player.unlockedWeapons.size===1&&g.state.player.unlockedWeapons.has(Weapon.FIST),'only fists initially');
ok(g.state.enemies.length===8,'opening population must be eight enemies');
ok(g.state.enemies.every(e=>!ranged.has(e.weapon)),'opening enemies must not have ranged weapons');
ok(/Сложность: Легко/.test(g.lastStatus),'opening must announce difficulty');
ok(/только кулаки/i.test(g.lastStatus),'opening must explain that player has only fists');

reach();
ok(g.state.campaign.stage===1,'stash advances');
ok(g.state.player.unlockedWeapons.has(Weapon.SHANK),'stash unlocks shank');
ok(!g.state.player.unlockedWeapons.has(Weapon.PISTOL),'pistol must stay locked after stash');

killObjective();
ok(g.state.campaign.stage===2,'key guard advances');
ok(g.state.player.unlockedWeapons.has(Weapon.KNIFE),'key unlocks knife');
ok(g.state.player.unlockedWeapons.has(Weapon.BATON),'key unlocks baton');
ok(g.state.player.unlockedWeapons.has(Weapon.PISTOL),'key unlocks pistol before first ranged defense');

reach();
ok(g.state.campaign.holdUntil>g.time,'control hold begins');
tick(3.1);
const controlEnemies=g.state.enemies.filter(e=>e.objectiveTag?.startsWith('control-wave'));
ok(controlEnemies.length>=1&&controlEnemies.length<=2,'first control wave must be small');
ok(controlEnemies.every(e=>e.weapon!==Weapon.AUTOMATIC&&e.weapon!==Weapon.SNIPER),'control must not spawn automatic or sniper enemies');
tick(73);
ok(g.state.campaign.stage===3,'control hold advances');
ok(g.state.player.unlockedWeapons.has(Weapon.TASER),'control unlocks taser');
ok(g.state.enemies.every(e=>!e.objectiveTag?.startsWith('control-wave')),'control survivors must be cleared when stage advances');
ok(g.state.enemies.length<=10,'yard stage must start with bounded enemy count');

killObjective();
ok(g.state.campaign.stage===4,'yard shooter advances');
ok(g.state.player.unlockedWeapons.has(Weapon.CROWBAR),'yard unlocks crowbar');
ok(g.state.player.unlockedWeapons.has(Weapon.AUTOMATIC),'yard unlocks automatic before armory captain');
ok(g.state.enemies.every(e=>!String(e.id).includes('yard')),'yard survivors must be cleared before armory');
ok(g.state.enemies.length<=11,'armory stage must keep local enemy count bounded');

killObjective();
ok(g.state.campaign.stage===5,'armory captain advances');
ok(g.state.player.unlockedWeapons.has(Weapon.BAT),'armory unlocks bat');
ok(g.state.player.unlockedWeapons.has(Weapon.MACHETE),'armory unlocks machete');
ok(g.state.enemies.length===8,'armory survivors must be cleared after stage completion');

reach();
ok(g.state.campaign.holdUntil>g.time,'generator hold begins');
tick(91);
ok(g.state.campaign.stage===6,'generator advances');
ok(g.state.player.unlockedWeapons.has(Weapon.SNIPER),'generator unlocks sniper before warden');
ok(g.state.player.unlockedWeapons.size===11,'all 11 weapons unlocked progressively before warden/exit');
ok(g.state.enemies.every(e=>!e.objectiveTag?.startsWith('generator-wave')),'generator survivors must be cleared before warden');
ok(g.state.enemies.length<=12,'warden stage must keep enemy count bounded');

killObjective();
ok(g.state.campaign.stage===7,'warden advances to exit');
ok(g.state.enemies.length===8,'warden escorts must not follow player into extraction');
reach();
ok(!g.state.campaign.completed,'exit stays locked before 20 minutes');
g.time=1200;g.emit('update',{dt:.1,time:g.time});
ok(g.state.campaign.completed,'campaign completes at or after 20 minutes');
ok(g.state.mode==='victory','victory mode');
ok(g.state.enemies.length===8,'victory must not retain scripted reinforcement crowds');

console.log('Campaign balanced playthrough passed:',{time:g.time,stages:8,weapons:g.state.player.unlockedWeapons.size,enemies:g.state.enemies.length});
