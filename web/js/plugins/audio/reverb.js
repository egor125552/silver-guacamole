function impulse(ctx,seconds,decay,highCut=.65){
  const n=Math.floor(ctx.sampleRate*seconds),b=ctx.createBuffer(2,n,ctx.sampleRate);
  for(let ch=0;ch<2;ch++){
    const d=b.getChannelData(ch);let lp=0;
    for(let i=0;i<n;i++){
      const t=i/ctx.sampleRate;
      const white=(Math.random()*2-1)*Math.pow(1-t/seconds,decay);
      lp=lp*highCut+white*(1-highCut);
      d[i]=lp;
    }
  }
  return b;
}

export default{
  id:"reverb",
  install(game){
    const c=game.audio.ctx;
    const send=c.createGain();
    send.gain.value=1;

    // The reverb listens to the common environment bus, never to individual sounds.
    // Therefore every voice receives exactly the same room-routing rules.
    game.audio.environmentInput.connect(send);

    const tone=c.createBiquadFilter();
    tone.type="lowpass";
    tone.frequency.value=12500;
    tone.Q.value=.25;
    send.connect(tone);

    const profiles={
      generic:{buffer:impulse(c,2.15,1.7,.60),wet:.48,cutoff:12500},
      cave:{buffer:impulse(c,4.7,1.25,.78),wet:.70,cutoff:9200},
    };

    const makeSlot=()=>{
      const convolver=c.createConvolver();
      const gain=c.createGain();gain.gain.value=0;
      tone.connect(convolver);convolver.connect(gain);gain.connect(game.audio.master);
      return{convolver,gain};
    };
    const slots=[makeSlot(),makeSlot()];
    let active=0,current="generic";
    slots[0].convolver.buffer=profiles.generic.buffer;
    slots[0].gain.gain.value=profiles.generic.wet;

    game.setRoomReverb=(name)=>{
      const profile=profiles[name];
      if(!profile||name===current)return;
      const now=c.currentTime,next=1-active;
      const outgoing=slots[active],incoming=slots[next];
      incoming.convolver.buffer=profile.buffer;

      outgoing.gain.gain.cancelScheduledValues(now);
      incoming.gain.gain.cancelScheduledValues(now);
      outgoing.gain.gain.setValueAtTime(outgoing.gain.gain.value,now);
      incoming.gain.gain.setValueAtTime(0,now);
      outgoing.gain.gain.linearRampToValueAtTime(0,now+.28);
      incoming.gain.gain.linearRampToValueAtTime(profile.wet,now+.28);
      tone.frequency.cancelScheduledValues(now);
      tone.frequency.setValueAtTime(tone.frequency.value,now);
      tone.frequency.linearRampToValueAtTime(profile.cutoff,now+.28);

      active=next;current=name;game.state.room=name;
    };

    return()=>{
      delete game.setRoomReverb;
      try{game.audio.environmentInput.disconnect(send);}catch{}
      try{send.disconnect();tone.disconnect();}catch{}
      for(const slot of slots){try{slot.convolver.disconnect();slot.gain.disconnect();}catch{}}
    };
  }
};
