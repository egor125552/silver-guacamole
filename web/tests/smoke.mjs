import {EventBus} from '../js/core/events.js';
import {Settings,Weapon} from '../js/core/constants.js';
import {hasLineOfSight} from '../js/core/math.js';

const base=new URL('../js/plugins/',import.meta.url);
const names=[
 'world/level.js','player/state.js','player/damage.js','player/combat-state.js','player/regen.js','player/dodge.js','player/footsteps.js','player/shadow.js','player/low-health.js','player/death-state.js',
 'enemy/loadouts.js','enemy/population.js','enemy/damage.js','enemy/spotting.js','enemy/vision.js','enemy/hearing.js','enemy/social.js','enemy/tactics.js','enemy/navigation.js','enemy/combat-memory.js','enemy/patrol.js','enemy/alert.js','enemy/search.js','enemy/melee-combat.js','enemy/ranged-combat.js','enemy/footsteps.js','enemy/respawn.js',
 'combat/weapons.js','combat/player-attack.js','combat/automatic-fire.js','combat/takedown.js','sonar/full.js','sonar/directional.js','sonar/proximity.js','ambience/npc-scenes.js','input/movement.js'
];
class Game{
 constructor(){this.bus=new EventBus();this.settings={...Settings};this.time=0;this.state={mode:'playing',player:null,enemies:[],walls:[],shadowZones:[],room:'generic'};this.sounds=[];this.audio={play:(n,o={})=>{this.sounds.push([n,o]);return{};},startLoop:(n,k,o={})=>this.sounds.push(['loop:'+n,{key:k,...o}]),stopLoop:k=>this.sounds.push(['stop:'+k,{}])};this.input={held:new Set(),has(...k){return k.some(x=>this.held.has(x));}};}
 on(n,f){return this.bus.on(n,f)}emit(n,p){this.bus.emit(n,p)}hasLOS(a,b){return hasLineOfSight(a,b,this.state.walls)}status(t){this.lastStatus=t}
}
const g=new Game();for(const name of names){const m=await import(new URL(name,base));await m.default.install(g);}g.emit('reset');
const ok=(c,m)=>{if(!c)throw new Error(m)};
ok(g.state.player.health===250,'player reset');ok(g.state.enemies.length===20,'enemy count');ok(g.state.walls.length===25,'wall count');
const z=g.state.player.position.z;g.input.held.add('ArrowUp');g.emit('update',{dt:.1,time:.1});g.input.held.clear();ok(g.state.player.position.z<z,'movement');
const e=g.state.enemies[0];e.alive=true;e.role="investigator";e.position={x:8,z:0};e.state='patrolling';e.detection=0;g.state.player.position={x:0,z:0};g.state.walls=[];g.emit('playerNoise',{position:{x:0,z:0},radius:10});ok(e.state==='alert','open hearing');
e.state='patrolling';e.detection=0;g.state.walls=[{x:3,z:-2,w:2,h:4}];g.emit('playerNoise',{position:{x:0,z:0},radius:10});ok(e.state==='patrolling','wall hearing');
g.state.walls=[];e.position={x:1,z:0};e.state='patrolling';e.alive=true;e.health=e.maxHealth=120;g.state.player.position={x:0,z:0};g.state.player.weapon=Weapon.FIST;g.state.player.lastAttackTime=-10;g.time=10;g.emit('playerAttack');ok(e.health===85,'fist damage');
e.alive=true;e.health=120;e.state='patrolling';e.detection=0;e.position={x:1,z:0};g.state.player.lastAttackTime=0;g.time=20;g.emit('playerTakedown');ok(!e.alive,'takedown');
g.sounds=[];g.state.player.position={x:0,z:0};g.state.walls=[{x:5,z:-5,w:2,h:10}];g.time=30;g.emit('fullSonar');ok(g.sounds.some(x=>x[0]==='sonar'),'sonar');ok(g.sounds.some(x=>x[0]==='sonar_echo'),'sonar echo');
g.state.player.health=200;g.state.player.alive=true;g.state.player.lastDamageTime=-99;g.time=40;ok(g.damagePlayer(20),'damage');ok(g.state.player.health===180,'damage amount');g.time=49;g.emit('update',{dt:1,time:49});ok(g.state.player.health>=185,'regen');
console.log('Stealth Action Web smoke tests passed');
