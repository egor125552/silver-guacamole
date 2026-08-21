export default {
  id:"room-reverb",
  install(game){
    let cave=false;
    return game.on("update",()=>{
      const p=game.state.player;if(!p)return;
      const next=p.position.x>50;
      if(next!==cave){ cave=next; game.setRoomReverb?.(cave?"cave":"generic"); }
    });
  }
};
