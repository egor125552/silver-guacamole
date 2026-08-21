import {distance,rand} from "../../core/math.js";import {randomTarget} from "./population.js";
export default{id:"enemy-patrol",install(game){return game.on("update",({dt})=>{if(game.time<game.settings.gracePeriod)return;for(const e of game.state.enemies){if(!e.alive||game.time<e.stunnedUntil||e.state!=="patrolling")continue;
  if(e.role==="sentry"||e.role==="idle"){
    if(game.time>=e.decisionAt){e.decisionAt=game.time+rand(4.5,10);if(e.role==="idle"&&Math.random()<.28){e.waiting=false;e.target=randomTarget(game);}else e.waiting=true;}
    if(e.waiting)continue;
  }
  if(e.waiting){if(game.time-e.stateSince>rand(2.5,6.5)){e.waiting=false;e.target=randomTarget(game);e.decisionAt=game.time+rand(8,14);}}
  else{const d=distance(e.position,e.target);if(d>1){const moved=game.moveEnemy(e,{x:e.target.x-e.position.x,z:e.target.z-e.position.z},e.walkSpeed,dt);game.emit("enemyMoved",{enemy:e,running:false,volume:64+(Number(e.id)||0)%4*4,moved});}else{e.waiting=true;e.stateSince=game.time;}if(game.time>=e.decisionAt){e.waiting=Math.random()<.45;e.stateSince=game.time;e.target=randomTarget(game);e.decisionAt=game.time+rand(8,15);}}
}});}};
