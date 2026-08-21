import {clamp,distance,pointInRect} from "../../core/math.js";
export default {
  id:"enemy-vision",
  install(game){
    return game.on("update",({dt})=>{
      const p=game.state.player;if(!p?.alive)return;
      for(const e of game.state.enemies){
        if(!e.alive||e.state==="combat")continue;
        const d=distance(e.position,p.position);
        let range=e.type==="guard"?48:e.type==="shooter"?55:e.type==="boss"?60:36;
        const los=d<=range&&game.hasLOS(e.position,p.position);
        if(los){
          let rate=25;if(d<5)rate*=3;else if(d>range*.65)rate*=.45;else if(d>20)rate*=.7;
          if(game.state.shadowZones.some(z=>pointInRect(p.position,z)))rate*=.4;
          if(p.running)rate*=1.5;else if(p.crouching)rate*=.6;
          e.detection+=rate*dt;
        } else e.detection-=14*dt;
        e.detection=clamp(e.detection,0,100);
        if(e.detection>=100)game.spotEnemy(e);
      }
    });
  }
};
