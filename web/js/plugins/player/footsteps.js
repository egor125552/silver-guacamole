import {PlayerSpeeds} from "../../core/constants.js";
import {rand} from "../../core/math.js";
export default {
  id:"player-footsteps",
  install(game){
    let next=0;
    const reset=game.on("reset",()=>next=0);
    const update=game.on("update",()=>{
      const p=game.state.player;if(!p?.alive||!p.moving)return;
      const interval=p.crouching?PlayerSpeeds.CROUCH_STEP:(p.running?PlayerSpeeds.RUN_STEP:PlayerSpeeds.WALK_STEP);
      if(game.time<next)return;
      const volume=p.crouching?62:(p.running?112:96);
      const pitch=p.running?rand(1.02,1.08):rand(.96,1.04);
      p.footSide*=-1;
      // Alternate real left/right feet below the listener so HRTF can encode both side and height.
      const position={x:p.position.x+p.footSide*.22,y:0,z:p.position.z+.18};
      game.audio.play("footstep",{position,relative:false,volume,pitch,spatialMode:"hrtf"});
      const noise=p.running?25:(p.crouching?3:10);
      game.emit("playerNoise",{position:{...p.position},radius:noise});
      next=game.time+interval;
    });
    return()=>{reset();update();};
  }
};
