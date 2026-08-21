import {distance} from "../../core/math.js";
export default{
  id:"player-combat-state",
  install(game){return game.on("update",()=>{const p=game.state.player;if(!p?.inCombat)return;const near=game.state.enemies.some(e=>e.alive&&e.state==="combat"&&distance(e.position,p.position)<25);if(near)p.lastCombatTime=game.time;else if(game.time-p.lastCombatTime>10)p.inCombat=false;});}
};
