import {distance} from "../../core/math.js";
export default{
  id:"proximity-sonar",
  install(game){
    let next=0;
    return game.on("update",()=>{
      const p=game.state.player;if(!p?.alive)return;
      let closest=null,best=Infinity;
      for(const e of game.state.enemies){if(!e.alive)continue;const d=distance(p.position,e.position);if(d<best){best=d;closest=e;}}
      if(!closest||best>50)return;
      const factor=1-best/50,delay=Math.max(.14,1.45*(1-factor*.91));
      if(game.time<next)return;
      game.audio.play("EnemyPing",{position:closest.position,volume:72+factor*45,pitch:.92+factor*.72});
      next=game.time+delay;
    });
  }
};
