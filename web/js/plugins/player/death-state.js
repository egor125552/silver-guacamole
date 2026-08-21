export default {
  id:"death-state",
  install(game){
    return game.on("update",()=>{
      const p=game.state.player;if(!p)return;
      if(game.state.mode==="dying"&&game.time-p.deathAt>2.5){
        game.state.mode="gameover";
        game.audio.stopLoop("lowHealth");game.audio.stopLoop("shadow");
        game.status("Вы погибли. Нажмите R, чтобы начать заново.");
      }
    });
  }
};
