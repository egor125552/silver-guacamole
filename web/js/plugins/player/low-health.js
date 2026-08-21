export default {
  id:"low-health-pulse",
  install(game){
    const off=game.on("update",()=>{
      const p=game.state.player;if(!p)return;
      if(p.alive&&p.health<=game.settings.lowHealthThreshold) game.audio.startLoop("LowHealth","lowHealth",{relative:true,volume:80,pitch:1});
      else game.audio.stopLoop("lowHealth");
    });
    const off2=game.on("reset",()=>game.audio.stopLoop("lowHealth"));
    return ()=>{off();off2();game.audio.stopLoop("lowHealth");};
  }
};
