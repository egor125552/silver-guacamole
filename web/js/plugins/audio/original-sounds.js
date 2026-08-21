const soundUrl=path=>new URL(`../../../../sounds/${path}`,import.meta.url).href;
const ORIGINAL={
  automatic:"automatic.ogg",battle_cry:"battle_cry.ogg",boss_hit:"boss_hit.ogg",death:"death.ogg",footstep:"footstep.ogg",
  hit:"hit.ogg",miss:"miss.ogg",pistol:"pistol.ogg",player_death:"player_death.ogg",player_hit:"player_hit.ogg",punch:"punch.ogg",
  reaction_panic:"reaction_panic.ogg",sniper:"sniper.ogg",sonar:"sonar.ogg",takedown:"takedown.ogg",ultimate:"ultimate.ogg",
  guard_vs_lukashenko_main:"guard_vs_lukashenko/main_event.ogg",prisoners_arguing_main:"prisoners_arguing/main_event.ogg"
};
function mono(ctx,b){
  if(b.numberOfChannels===1)return b;
  const out=ctx.createBuffer(1,b.length,b.sampleRate),d=out.getChannelData(0);d.fill(0);
  for(let c=0;c<b.numberOfChannels;c++){const s=b.getChannelData(c);for(let i=0;i<d.length;i++)d[i]+=s[i]/b.numberOfChannels;}
  return out;
}
export default{
  id:"original-sounds",
  async install(game){
    await Promise.all(Object.entries(ORIGINAL).map(async([name,path])=>{
      try{const r=await fetch(soundUrl(path));if(!r.ok)throw new Error(`${r.status} ${path}`);const raw=await game.audio.ctx.decodeAudioData(await r.arrayBuffer());game.audio.setBuffer(name,mono(game.audio.ctx,raw));}
      catch(e){console.warn("Не удалось загрузить звук",name,e);}
    }));
  }
};
