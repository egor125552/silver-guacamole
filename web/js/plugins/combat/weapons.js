import {Weapon} from "../../core/constants.js";
export default{
  id:"weapon-profiles",
  install(game){game.weaponProfile=type=>{const s=game.settings;switch(type){
    case Weapon.FIST:return{damage:s.fistDamage,cooldown:.5,sound:"punch",automatic:false,volume:s.fistVolume,range:1.8};
    case Weapon.PISTOL:return{damage:s.pistolDamage,cooldown:.4,sound:"pistol",automatic:false,volume:s.pistolVolume,range:32};
    case Weapon.TASER:return{damage:s.taserDamage,cooldown:s.taserCooldown,sound:"Taser_Fire",automatic:false,volume:s.taserVolume,range:s.taserRange};
    case Weapon.AUTOMATIC:return{damage:s.automaticDamage,cooldown:.12,sound:"automatic",automatic:true,volume:s.automaticVolume,range:28};
    case Weapon.SNIPER:return{damage:s.sniperDamage,cooldown:1.5,sound:"sniper",automatic:false,volume:s.sniperVolume,range:75};
    case Weapon.MACHETE:return{damage:s.macheteDamage,cooldown:.7,sound:"Machete_Swish",automatic:false,volume:s.macheteVolume,range:2.2};
    case Weapon.KNIFE:return{damage:s.knifeDamage,cooldown:.4,sound:"Knife_Swish",automatic:false,volume:s.knifeVolume,range:1.7};
    case Weapon.CROWBAR:return{damage:s.crowbarDamage,cooldown:.9,sound:"Blunt_Metal_Swish",automatic:false,volume:s.crowbarVolume,range:2.1};
    case Weapon.BAT:return{damage:s.batDamage,cooldown:.8,sound:"Bat_Swish",automatic:false,volume:s.batVolume,range:2.3};
    case Weapon.SHANK:return{damage:s.shankDamage,cooldown:.3,sound:"Knife_Swish",automatic:false,volume:s.shankVolume,range:1.6};
    case Weapon.BATON:return{damage:s.batonDamage,cooldown:.6,sound:"Blunt_Metal_Swish",automatic:false,volume:s.batonVolume,range:2};
  }};}
};
