export default{
  id:"player-dodge",
  install(game){
    const off1=game.on("playerDodge",()=>{const p=game.state.player;if(!p?.alive||!p.inCombat||p.dodging||game.time<p.dodgeReadyAt)return;p.dodging=true;p.dodgeUntil=game.time+.45;p.dodgeReadyAt=game.time+1.25;game.audio.play("Dodge",{relative:true,volume:90});});
    const off2=game.on("update",()=>{const p=game.state.player;if(p?.dodging&&game.time>=p.dodgeUntil)p.dodging=false;});return()=>{off1();off2();};
  }
};
