export default {
  id:"health-regen",
  install(game){
    return game.on("update",({dt})=>{
      const p=game.state.player,s=game.settings;
      if(!p?.alive||p.health>=p.maxHealth||game.time<p.regenReadyAt)return;
      p.regenBuffer+=s.healthRegenRate*dt;
      if(p.regenBuffer>=1){
        const n=Math.floor(p.regenBuffer);
        p.health=Math.min(p.maxHealth,p.health+n);p.regenBuffer-=n;
      }
    });
  }
};
