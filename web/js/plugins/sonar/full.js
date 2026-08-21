import {rayRect} from "../../core/math.js";
export default{
  id:"full-sonar",
  install(game){
    let readyAt=0;
    return game.on("fullSonar",()=>{
      if(game.time<readyAt)return;readyAt=game.time+2;
      const p=game.state.player;game.audio.play("sonar",{relative:true,volume:80,pitch:1});
      for(let i=0;i<36;i++){
        const a=i/36*Math.PI*2,d={x:Math.cos(a),z:Math.sin(a)};let hit=null,best=Infinity;
        for(const w of game.state.walls){const h=rayRect(p.position,d,w);if(h&&h.t<best){best=h.t;hit=h;}}
        if(hit&&best<40)game.audio.play("sonar_echo",{position:{x:hit.x,z:hit.z},volume:100*(1-best/40),pitch:1.5-best/40});
      }
    });
  }
};
