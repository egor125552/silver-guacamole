function impulse(ctx, seconds, decay, highCut=.65) {
  const n=Math.floor(ctx.sampleRate*seconds), b=ctx.createBuffer(2,n,ctx.sampleRate);
  for(let ch=0;ch<2;ch++){
    const d=b.getChannelData(ch); let lp=0;
    for(let i=0;i<n;i++){
      const t=i/ctx.sampleRate;
      const white=(Math.random()*2-1)*Math.pow(1-t/seconds,decay);
      lp=lp*highCut+white*(1-highCut);
      d[i]=lp;
    }
  }
  return b;
}
export default {
  id:"reverb",
  install(game){
    const c=game.audio.ctx;
    const input=c.createGain(), convolver=c.createConvolver(), wet=c.createGain();
    wet.gain.value=.72;
    input.connect(convolver); convolver.connect(wet); wet.connect(game.audio.master);
    const profiles={
      generic: impulse(c,2.15,1.7,.60),
      cave: impulse(c,4.7,1.25,.78),
    };
    convolver.buffer=profiles.generic;
    game.audio.setReverbInput(input);
    game.setRoomReverb=(name)=>{
      if(!profiles[name]) return;
      convolver.buffer=profiles[name];
      game.state.room=name;
    };
    return ()=>{ input.disconnect(); convolver.disconnect(); wet.disconnect(); };
  }
};
