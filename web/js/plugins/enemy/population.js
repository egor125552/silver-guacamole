import {NPCType,AIState} from "../../core/constants.js";import {rand,randInt,distance,entityRect,rectIntersects} from "../../core/math.js";
function blocked(game,pos){const b=entityRect(pos,.5);return game.state.walls.some(w=>rectIntersects(b,w));}
export function randomTarget(game){const s=game.settings;return{x:rand(-s.worldSize,s.worldSize),z:rand(-s.worldSize,s.worldSize)};}
export function safeSpawn(game,min,around=null){const s=game.settings,p=game.state.player.position;for(let i=0;i<500;i++){const pos=around?{x:around.x+rand(-12,12),z:around.z+rand(-12,12)}:{x:rand(-s.worldSize+1,s.worldSize-1),z:rand(-s.worldSize+1,s.worldSize-1)};pos.x=Math.max(-s.worldSize+1,Math.min(s.worldSize-1,pos.x));pos.z=Math.max(-s.worldSize+1,Math.min(s.worldSize-1,pos.z));if(distance(pos,p)>=min&&!blocked(game,pos))return pos;}return{x:s.worldSize*.75,z:s.worldSize*.75};}
function roleFor(type,index){
  if(type===NPCType.BOSS)return "boss";
  if(type===NPCType.SHOOTER)return index%2?"watcher":"support";
  if(type===NPCType.GUARD){const r=index%4;return r===0?"sentry":r===1?"investigator":r===2?"charger":"patroller";}
  const r=index%5;return r===0?"idle":r===1?"investigator":r===2?"charger":"patroller";
}
export function makeEnemy(game,{id,type=NPCType.REGULAR,position,role=null,objectiveTag=null,health=null,weapon=null,behavior=null}={}){
  const l=game.enemyLoadout(type),r=role||roleFor(type,Number.isFinite(id)?id:randInt(0,99));
  const speedJitter=.88+Math.random()*.24;
  return{id:id??`spawn-${Math.random().toString(36).slice(2)}`,type,role:r,objectiveTag,position:position||safeSpawn(game,22),alive:true,
    health:health??l.maxHealth,maxHealth:health??l.maxHealth,weapon:weapon??l.weapon,behavior:behavior??l.behavior,
    state:AIState.PATROLLING,detection:0,hasReactedToDeath:false,target:null,waiting:r==="idle"||r==="sentry",
    decisionAt:game.time+rand(2,6),stateSince:game.time,nextStep:game.time+rand(.05,.9),lastAttack:game.time,
    stunnedUntil:0,deathTime:-999,walkSpeed:game.settings.npcWalkSpeed*speedJitter,
    runSpeed:game.settings.npcRunSpeed*(r==="charger"?1.15:speedJitter),stepPhase:rand(0,.3),stepPitch:rand(.90,1.10)};
}
export default{id:"enemy-population",install(game){game.spawnEnemy=spec=>{const e=makeEnemy(game,spec);e.target=randomTarget(game);game.state.enemies.push(e);return e;};return game.on("reset",()=>{game.state.enemies=[];for(let i=0;i<20;i++){let type=i%4===0?NPCType.GUARD:NPCType.REGULAR;if(i===18)type=NPCType.SHOOTER;game.spawnEnemy({id:i,type,position:safeSpawn(game,22)});}});}};
