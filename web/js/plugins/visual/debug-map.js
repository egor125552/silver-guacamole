export default{
  id:"debug-map",
  install(game){
    const canvas=document.querySelector("#debug-map");
    if(!canvas)return;
    const ctx=canvas.getContext("2d");
    const draw=()=>{
      const p=game.state.player;if(!p)return;
      const sx=canvas.width/80,sz=canvas.height/60;
      const X=x=>(x-p.position.x+40)*sx,Z=z=>(z-p.position.z+30)*sz;
      ctx.fillStyle="rgb(10,10,20)";ctx.fillRect(0,0,canvas.width,canvas.height);
      ctx.fillStyle="rgb(100,120,140)";
      for(const w of game.state.walls)ctx.fillRect(X(w.x),Z(w.z),w.w*sx,w.h*sz);
      for(const e of game.state.enemies){
        if(!e.alive)continue;
        ctx.fillStyle=e.state==="combat"?"red":(e.state==="alert"||e.state==="searching"?"yellow":"rgb(200,200,200)");
        ctx.beginPath();ctx.arc(X(e.position.x),Z(e.position.z),4,0,Math.PI*2);ctx.fill();
      }
      ctx.fillStyle=p.alive?"green":"rgb(100,100,100)";ctx.beginPath();ctx.arc(X(p.position.x),Z(p.position.z),4,0,Math.PI*2);ctx.fill();
    };
    return game.on("update",draw);
  }
};
