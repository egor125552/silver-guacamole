export default{id:"input-attack",install(game){return game.on("keyDown",({key,code})=>{if(game.state.mode==="playing"&&(key==="Space"||code==="Space"))game.emit("playerAttack");});}};
