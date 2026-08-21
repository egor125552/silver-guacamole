class BrowserAudio {
  constructor(game) {
    this.game=game;
    const Ctx=globalThis.AudioContext||globalThis.webkitAudioContext;
    if(!Ctx) throw new Error("Web Audio API недоступен в этом браузере");
    this.ctx=new Ctx();
    this.master=this.ctx.createGain();
    this.master.gain.value=.9;
    this.master.connect(this.ctx.destination);
    this.buffers=new Map();
    this.reverbInput=null;
    this.reverbEnabled=true;
  }
  setBuffer(name,buffer){this.buffers.set(name,buffer);}
  setReverbInput(node){this.reverbInput=node;}
  setListener(position){
    const l=this.ctx.listener,t=this.ctx.currentTime;
    if(l.positionX){
      l.positionX.setValueAtTime(position.x,t);l.positionY.setValueAtTime(0,t);l.positionZ.setValueAtTime(position.z,t);
      l.forwardX.setValueAtTime(0,t);l.forwardY.setValueAtTime(0,t);l.forwardZ.setValueAtTime(-1,t);
      l.upX.setValueAtTime(0,t);l.upY.setValueAtTime(1,t);l.upZ.setValueAtTime(0,t);
    }else{l.setPosition(position.x,0,position.z);l.setOrientation(0,0,-1,0,1,0);}
  }
  play(name,{position={x:0,z:0},volume=100,relative=false,pitch=null,loop=false,wet=true}={}){
    const buffer=this.buffers.get(name);if(!buffer)return null;
    const src=this.ctx.createBufferSource();src.buffer=buffer;src.loop=loop;
    src.playbackRate.value=pitch??(.97+Math.random()*.06);
    const gain=this.ctx.createGain();gain.gain.value=Math.max(0,Math.min(1.25,volume/100));src.connect(gain);
    let tail=gain;
    if(!relative){
      const p=this.ctx.createPanner();p.panningModel="HRTF";p.distanceModel="inverse";
      p.refDistance=name==="EnemyPing"?11:4;p.rolloffFactor=name==="EnemyPing"?.62:1.5;p.maxDistance=name==="EnemyPing"?60:10000;
      if(p.positionX){p.positionX.value=position.x;p.positionY.value=0;p.positionZ.value=position.z;}else p.setPosition(position.x,0,position.z);
      gain.connect(p);tail=p;
    }
    tail.connect(this.master);
    if(!relative&&wet&&this.reverbEnabled&&this.reverbInput)tail.connect(this.reverbInput);
    src.start();return src;
  }
}
export default {id:"audio-core",async install(game){game.audio=new BrowserAudio(game);await game.audio.ctx.resume();return()=>game.audio.ctx.close();}};
