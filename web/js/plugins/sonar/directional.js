import {rayRect} from "../../core/math.js";
const pi=Math.PI,angles={8:-pi/2,9:-pi/4,6:0,3:pi/4,2:pi/2,1:3*pi/4,4:pi,7:-3*pi/4};
export default{
  id:"directional-sonar",
  install(game){
    let readyAt=0;
    return game.on("directionalSonar",key=>{
      if(game.time<readyAt||angles[key]===undefined)return;readyAt=game.time+.5;
      const a=angles[key],d={x:Math.cos(a),z:Math.sin(a)},p=game.state.player;let hit=null,best=Infinity;
      game.audio.play("sonar",{relative:true,volume:40,pitch:1});
      for(const w of game.state.walls){const h=rayRect(p.position,d,w);if(h&&h.t<best){best=h.t;hit=h;}}
      if(hit&&best<60)game.audio.play("sonar_echo",{position:{x:hit.x,z:hit.z},volume:100,pitch:1.8-best/60});
    });
  }
};
