class BrowserAudio {
  constructor(game) {
    this.game=game;
    const Ctx=globalThis.AudioContext||globalThis.webkitAudioContext;
    if(!Ctx) throw new Error("Web Audio API недоступен в этом браузере");
    this.ctx=new Ctx();

    // Every sound ends up here. Nothing is allowed to connect directly to the destination.
    // This gives the game one predictable processing path:
    // source -> optional spatial stage -> environmentInput -> dry/reverb -> master -> output.
    this.master=this.ctx.createGain();
    this.master.gain.value=.9;
    this.master.connect(this.ctx.destination);

    this.environmentInput=this.ctx.createGain();
    this.dryBus=this.ctx.createGain();
    this.dryBus.gain.value=1;
    this.environmentInput.connect(this.dryBus);
    this.dryBus.connect(this.master);

    this.buffers=new Map();
    this.spatialize=null;
    this.reverbEnabled=true;
    this.listenerHeight=1.65;
  }
  setBuffer(name,buffer){this.buffers.set(name,buffer);}
  setSpatializer(fn){this.spatialize=typeof fn==="function"?fn:null;}
  setListener(position){
    const l=this.ctx.listener,t=this.ctx.currentTime,y=this.listenerHeight;
    if(l.positionX){
      l.positionX.setValueAtTime(position.x,t);l.positionY.setValueAtTime(y,t);l.positionZ.setValueAtTime(position.z,t);
      l.forwardX.setValueAtTime(0,t);l.forwardY.setValueAtTime(0,t);l.forwardZ.setValueAtTime(-1,t);
      l.upX.setValueAtTime(0,t);l.upY.setValueAtTime(1,t);l.upZ.setValueAtTime(0,t);
    }else{l.setPosition(position.x,y,position.z);l.setOrientation(0,0,-1,0,1,0);}
  }
  play(name,{position={x:0,z:0},volume=100,relative=false,pitch=null,loop=false,spatialMode="hybrid"}={}){
    const buffer=this.buffers.get(name);if(!buffer)return null;
    const src=this.ctx.createBufferSource();src.buffer=buffer;src.loop=loop;
    src.playbackRate.value=pitch??(.97+Math.random()*.06);

    const gain=this.ctx.createGain();
    gain.gain.value=Math.max(0,Math.min(1.25,volume/100));
    src.connect(gain);

    let tail=gain;
    if(this.spatialize){
      tail=this.spatialize(gain,{name,position,relative,spatialMode})||gain;
    }

    // One and only one destination for all voices. The reverb plugin taps this bus,
    // so footsteps, UI-relative cues, weapons, NPCs and sonar share the same room treatment.
    tail.connect(this.environmentInput);
    src.start();
    return src;
  }
  close(){
    try{this.environmentInput.disconnect();}catch{}
    try{this.dryBus.disconnect();}catch{}
    try{this.master.disconnect();}catch{}
    return this.ctx.close();
  }
}
export default {id:"audio-core",async install(game){game.audio=new BrowserAudio(game);await game.audio.ctx.resume();return()=>game.audio.close();}};
