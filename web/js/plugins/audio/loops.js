export default{
  id:"audio-loops",
  install(game){
    const loops=new Map();
    game.audio.startLoop=(name,key,opts={})=>{if(loops.has(key))return;const src=game.audio.play(name,{...opts,loop:true});if(!src)return;loops.set(key,src);src.onended=()=>{if(loops.get(key)===src)loops.delete(key);};};
    game.audio.stopLoop=key=>{const src=loops.get(key);if(!src)return;try{src.stop();}catch{}loops.delete(key);};
    return()=>{for(const src of loops.values())try{src.stop();}catch{}loops.clear();};
  }
};
