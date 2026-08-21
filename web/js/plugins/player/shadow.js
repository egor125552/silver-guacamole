import {pointInRect} from "../../core/math.js";
export default {
  id:"shadow-ambience",
  install(game){
    const off=game.on("update",()=>{
      const p=game.state.player;if(!p)return;
      const inShadow=game.state.shadowZones.some(z=>pointInRect(p.position,z));
      p.inShadow=inShadow;
      if(inShadow)game.audio.startLoop("Shadow_Ambience","shadow",{relative:true,volume:30,pitch:1});
      else game.audio.stopLoop("shadow");
    });
    const off2=game.on("reset",()=>game.audio.stopLoop("shadow"));
    return ()=>{off();off2();game.audio.stopLoop("shadow");};
  }
};
