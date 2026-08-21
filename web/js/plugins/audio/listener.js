export default {
  id:"listener",
  install(game){
    return game.on("update",()=>{
      const p=game.state.player;
      if(p) game.audio.setListener(p.position);
    });
  }
};
