export default{
  id:"enemy-spotting",
  install(game){
    game.spotEnemy=enemy=>{if(!enemy||enemy.state==="combat")return;enemy.state="combat";enemy.detection=0;game.audio.play("Spotted",{position:enemy.position,volume:85});if(Math.random()<=.70)game.audio.play("battle_cry",{position:enemy.position,volume:100,pitch:.9+Math.random()*.2});game.emit("enemySpotted",enemy);};
  }
};
