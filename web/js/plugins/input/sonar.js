const pad={Numpad8:8,Numpad9:9,Numpad6:6,Numpad3:3,Numpad2:2,Numpad1:1,Numpad4:4,Numpad7:7};
export default{id:"input-sonar",install(game){return game.on("keyDown",({key,code})=>{if(game.state.mode!=="playing")return;if(key==="e"||code==="KeyE")game.emit("fullSonar");if(pad[code])game.emit("directionalSonar",pad[code]);});}};
