import {rand, rectIntersects} from "../../core/math.js";

function generate(game) {
  const s=game.settings, walls=[], shadows=[
    {x:-50,z:-50,w:20,h:80},
    {x:30,z:20,w:50,h:15},
  ];
  let attempts=0;
  while(walls.length<s.wallCount && attempts<s.wallCount*20+100){
    attempts++;
    const w=rand(5,20),h=rand(5,20);
    const x=rand(-s.worldSize+w,s.worldSize-w),z=rand(-s.worldSize+h,s.worldSize-h);
    const wall={x,z,w,h},start={x:-5,z:-5,w:10,h:10};
    if(rectIntersects(wall,start)) continue;
    let severe=false;
    for(const e of walls){
      const ix=Math.max(0,Math.min(wall.x+wall.w,e.x+e.w)-Math.max(wall.x,e.x));
      const iz=Math.max(0,Math.min(wall.z+wall.h,e.z+e.h)-Math.max(wall.z,e.z));
      if(ix*iz>w*h*.45){severe=true;break;}
    }
    if(!severe) walls.push(wall);
  }
  game.state.walls=walls; game.state.shadowZones=shadows;
}
export default {id:"level",install(game){return game.on("reset",()=>generate(game));}};
