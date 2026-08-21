import {rand} from "../../core/math.js";
export default{
  id:"enemy-footsteps",
  install(game){
    return game.on("enemyMoved",({enemy,running=false,volume=70,moved=true})=>{
      if(!moved||!enemy?.alive||game.time<enemy.nextStep)return;
      const personality=enemy.role==="charger"?.86:enemy.role==="sentry"?1.18:enemy.role==="investigator"?.96:1;
      const base=(running?.40:.60)*personality;
      const pitch=(enemy.stepPitch||1)*rand(.97,1.03)*(running?1.04:1);
      game.audio.play("footstep",{position:{x:enemy.position.x,y:0,z:enemy.position.z},relative:false,volume,pitch,spatialMode:"hrtf"});
      enemy.nextStep=game.time+base*rand(.88,1.13)+(enemy.stepPhase||0);
      enemy.stepPhase=0;
    });
  }
};
