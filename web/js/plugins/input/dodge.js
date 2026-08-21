export default{id:"input-dodge",install(game){return game.on("keyDown",({key,code})=>{if(game.state.mode==="playing"&&(key==="x"||code==="KeyX"))game.emit("playerDodge");});}};
