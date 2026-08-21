export default{id:"input-takedown",install(game){return game.on("keyDown",({key,code})=>{if(game.state.mode==="playing"&&(key==="f"||code==="KeyF"))game.emit("playerTakedown");});}};
