export default{
  id:"enemy-damage",
  install(game){
    game.damageEnemy=(enemy,damage,{stun=false,attacker="player"}={})=>{
      if(!enemy?.alive)return false;
      game.audio.play("hit",{position:enemy.position,volume:100});enemy.health-=damage;
      if(stun){enemy.stunnedUntil=game.time+5;game.audio.play("Stun",{position:enemy.position,volume:100});}
      else if(Math.random()*100<game.settings.npcStunChanceOnDamage)enemy.stunnedUntil=game.time+game.settings.npcStunDuration;
      if(enemy.health<=0){enemy.health=0;enemy.alive=false;enemy.deathTime=game.time;game.audio.play("death",{position:enemy.position,volume:100});game.emit("enemyDied",enemy);return true;}
      if(attacker==="player"&&enemy.state!=="combat")game.spotEnemy?.(enemy);return true;
    };
  }
};
