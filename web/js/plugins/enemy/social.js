import {distance} from "../../core/math.js";
export default {
  id:"enemy-social",
  install(game){
    return game.on("update",()=>{
      if(game.time<game.settings.gracePeriod)return;
      for(const e of game.state.enemies){
        if(!e.alive)continue;
        if(e.state!=="combat"&&!e.hasReactedToDeath){
          const dead=game.state.enemies.find(o=>!o.alive&&distance(e.position,o.position)<15&&game.hasLOS(e.position,o.position));
          if(dead){game.audio.play("reaction_panic",{position:e.position,volume:100});e.state="alert";e.target={...dead.position};e.stateSince=game.time;e.hasReactedToDeath=true;}
        }
        if(e.state!=="combat"){
          const fighter=game.state.enemies.find(o=>o!==e&&o.alive&&o.state==="combat"&&distance(e.position,o.position)<20&&game.hasLOS(e.position,o.position));
          if(fighter)game.spotEnemy(e);
        }
      }
    });
  }
};
