import {NPCType,AIBehavior,Weapon} from "../../core/constants.js";import {randInt} from "../../core/math.js";
export default{
  id:"enemy-loadouts",
  install(game){game.enemyLoadout=type=>{const s=game.settings;let maxHealth=s.regularHealth,weapon=Weapon.FIST,behavior=AIBehavior.AGGRESSOR;
    if(type===NPCType.SHOOTER){maxHealth=s.shooterHealth;weapon=Weapon.PISTOL;behavior=AIBehavior.SUPPORT;}
    else if(type===NPCType.GUARD){maxHealth=s.shooterHealth;const r=randInt(1,100);if(r<=s.guardPistolChance){weapon=Weapon.PISTOL;behavior=AIBehavior.SUPPORT;}else if(r<=s.guardPistolChance+s.guardAutomaticChance){weapon=Weapon.AUTOMATIC;behavior=AIBehavior.SUPPORT;}else if(r<=s.guardPistolChance+s.guardAutomaticChance+s.guardTaserChance){weapon=Weapon.TASER;behavior=AIBehavior.SUPPORT;}else{weapon=Weapon.BATON;behavior=AIBehavior.AGGRESSOR;}}
    else if(type===NPCType.BOSS){maxHealth=s.bossHealth;weapon=Weapon.AUTOMATIC;behavior=AIBehavior.SUPPORT;}
    else{const r=randInt(1,100);if(r<=s.prisonerPistolChance){weapon=Weapon.PISTOL;behavior=AIBehavior.SUPPORT;}else{const m=randInt(1,5);if(m===1)weapon=Weapon.KNIFE;else if(m===2)weapon=Weapon.SHANK;else if(m===3)weapon=Weapon.BAT;else if(m===4)weapon=Weapon.CROWBAR;}}
    return{maxHealth,weapon,behavior};};}
};
