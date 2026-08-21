const url = new URL("../../../../config.ini", import.meta.url).href;
const map = {
  Health:["playerHealth","int"], PlayerRunSpeed:["playerRunSpeed","float"], HealthRegenRate:["healthRegenRate","float"],
  HealthRegenDelay:["healthRegenDelay","float"], LowHealthThreshold:["lowHealthThreshold","int"],
  RegularHealth:["regularHealth","int"], ShooterHealth:["shooterHealth","int"], BossHealth:["bossHealth","int"],
  MeleeNpcCanAttack:["meleeNpcCanAttack","bool"], NpcWalkSpeed:["npcWalkSpeed","float"], NpcRunSpeed:["npcRunSpeed","float"],
  NpcStunChanceOnDamage:["npcStunChanceOnDamage","int"], NpcStunDuration:["npcStunDuration","float"],
  FistDamage:["fistDamage","int"], PistolDamage:["pistolDamage","int"], AutomaticDamage:["automaticDamage","int"],
  SniperDamage:["sniperDamage","int"], FistVolume:["fistVolume","float"], PistolVolume:["pistolVolume","float"],
  AutomaticVolume:["automaticVolume","float"], SniperVolume:["sniperVolume","float"], TaserDamage:["taserDamage","int"],
  TaserCooldown:["taserCooldown","float"], TaserRange:["taserRange","float"], TaserVolume:["taserVolume","float"],
  WorldSize:["worldSize","float"], WallCount:["wallCount","int"], GracePeriod:["gracePeriod","float"], RespawnTime:["respawnTime","float"],
  MacheteDamage:["macheteDamage","int"], MacheteVolume:["macheteVolume","float"], KnifeDamage:["knifeDamage","int"],
  KnifeVolume:["knifeVolume","float"], CrowbarDamage:["crowbarDamage","int"], CrowbarVolume:["crowbarVolume","float"],
  BatDamage:["batDamage","int"], BatVolume:["batVolume","float"], ShankDamage:["shankDamage","int"], ShankVolume:["shankVolume","float"],
  BatonDamage:["batonDamage","int"], BatonVolume:["batonVolume","float"], GuardPistolChance:["guardPistolChance","int"],
  GuardAutomaticChance:["guardAutomaticChance","int"], GuardTaserChance:["guardTaserChance","int"], PrisonerPistolChance:["prisonerPistolChance","int"]
};
function convert(value,type){
  if(type==="bool")return /^(yes|true|1)$/i.test(value);
  const n=type==="int"?parseInt(value,10):parseFloat(value);
  return Number.isFinite(n)?n:null;
}
export default {
  id:"config-loader",
  async install(game){
    try{
      const r=await fetch(url); if(!r.ok)return;
      const text=await r.text();
      for(let line of text.split(/\r?\n/)){
        line=line.split(";")[0].trim();if(!line.includes("="))continue;
        const [rawKey,...rest]=line.split("="),key=rawKey.trim(),value=rest.join("=").trim();
        const spec=map[key];if(!spec)continue;
        const parsed=convert(value,spec[1]);if(parsed!==null)game.settings[spec[0]]=parsed;
      }
      game.settings.taserRange=Math.max(1,Math.min(12,game.settings.taserRange));
    }catch(e){console.warn("config.ini not loaded, defaults used",e);}
  }
};
