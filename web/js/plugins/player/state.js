import {Weapon} from "../../core/constants.js";
export default {
  id:"player-state",
  install(game){
    return game.on("reset",()=>{
      const s=game.settings;
      game.state.player={
        position:{x:0,z:0}, health:s.playerHealth,maxHealth:s.playerHealth,alive:true,
        running:false,crouching:false,moving:false,godMode:false,weapon:Weapon.FIST,
        inCombat:false,dodging:false,dodgeUntil:0,dodgeReadyAt:1.25,stunnedUntil:0,
        lastAttackTime:0,lastDamageTime:-99,lastCombatTime:0,regenReadyAt:s.healthRegenDelay,
        regenBuffer:0,deathAt:0
      };
    });
  }
};
