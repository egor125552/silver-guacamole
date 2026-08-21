import {distance,wallsBetween,clamp} from "../../core/math.js";
export default {
  id:"enemy-hearing",
  install(game){
    return game.on("playerNoise",({position,radius})=>{
      for(const e of game.state.enemies){
        if(!e.alive||e.state==="combat")continue;
        const d=distance(position,e.position);
        const hearing=e.type==="guard"?1.15:e.type==="shooter"?1.05:e.type==="boss"?1.25:1;
        const walls=Math.min(3,wallsBetween(position,e.position,game.state.walls));
        const effective=radius*hearing*Math.pow(.48,walls);
        if(d>=effective)continue;
        const proximity=1-clamp(d/Math.max(effective,.01),0,1);
        const chance=e.role==="investigator"?1:e.role==="charger"?.88:e.role==="sentry"?.42:e.role==="idle"?.32:.70;
        if(Math.random()>chance&&proximity<.70){e.detection=clamp(e.detection+5*proximity,0,70);continue;}
        e.state="alert";e.target={...position};e.stateSince=game.time;e.decisionAt=game.time;
        e.detection=clamp(e.detection+12*proximity,0,85);
      }
    });
  }
};
