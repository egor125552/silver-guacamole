export default{
  id:"player-damage",
  install(game){
    game.damagePlayer=(damage,{stun=false}={})=>{
      const p=game.state.player;if(!p?.alive||p.godMode||p.dodging||game.time-p.lastDamageTime<.2)return false;
      p.lastDamageTime=game.time;p.regenReadyAt=game.time+game.settings.healthRegenDelay;p.regenBuffer=0;p.inCombat=true;p.lastCombatTime=game.time;
      if(stun){p.stunnedUntil=game.time+5;game.audio.play("Stun",{relative:true,volume:100});}
      game.audio.play("player_hit",{relative:true,volume:100});
      const hp=Math.max(0,p.health-damage);game.audio.play("HealthIndicator",{relative:true,volume:100,pitch:.5+hp/p.maxHealth});p.health=hp;
      if(p.health<=0){p.alive=false;game.state.mode="dying";p.deathAt=game.time;game.audio.play("player_death",{relative:true,volume:100});game.emit("playerDied");}
      return true;
    };
  }
};
