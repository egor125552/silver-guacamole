import {distance} from "../../core/math.js";
export default{
  id:"npc-ambient-scenes",
  install(game){
    let last=0;
    const off1=game.on("reset",()=>last=0);
    const off2=game.on("update",()=>{
      if(game.time-last<38)return;
      let guard=null,p1=null,p2=null;
      for(const e of game.state.enemies){
        if(!e.alive||e.state==="combat"||e.state==="alert")continue;
        if(e.type==="guard"&&!guard)guard=e;
        if(e.type==="regular"){if(!p1)p1=e;else if(!p2)p2=e;}
      }
      let played=false;
      if(guard&&p1&&distance(guard.position,p1.position)<14){
        game.audio.play("guard_vs_lukashenko_main",{position:{x:(guard.position.x+p1.position.x)/2,z:(guard.position.z+p1.position.z)/2},volume:72,pitch:1});
        played=true;
      }else if(p1&&p2&&distance(p1.position,p2.position)<12){
        game.audio.play("prisoners_arguing_main",{position:{x:(p1.position.x+p2.position.x)/2,z:(p1.position.z+p2.position.z)/2},volume:68,pitch:1});
        played=true;
      }
      if(played||game.time-last>65)last=game.time;
    });
    return()=>{off1();off2();};
  }
};
