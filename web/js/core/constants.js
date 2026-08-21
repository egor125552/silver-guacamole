export const Settings = Object.freeze({
  playerHealth: 250,
  playerRunSpeed: 16,
  healthRegenRate: 5,
  healthRegenDelay: 8,
  lowHealthThreshold: 100,
  regularHealth: 120,
  shooterHealth: 150,
  bossHealth: 500,
  meleeNpcCanAttack: true,
  npcWalkSpeed: 1.5,
  npcRunSpeed: 14.5,
  npcStunChanceOnDamage: 40,
  npcStunDuration: 0.75,
  fistDamage: 35,
  pistolDamage: 40,
  automaticDamage: 25,
  sniperDamage: 65,
  fistVolume: 100,
  pistolVolume: 110,
  automaticVolume: 100,
  sniperVolume: 120,
  taserDamage: 0,
  taserCooldown: 3,
  taserRange: 5,
  taserVolume: 110,
  macheteDamage: 55,
  macheteVolume: 105,
  knifeDamage: 45,
  knifeVolume: 95,
  crowbarDamage: 50,
  crowbarVolume: 110,
  batDamage: 40,
  batVolume: 100,
  shankDamage: 30,
  shankVolume: 90,
  batonDamage: 35,
  batonVolume: 100,
  worldSize: 100,
  wallCount: 25,
  gracePeriod: 5,
  respawnTime: 30,
  guardPistolChance: 40,
  guardAutomaticChance: 20,
  guardTaserChance: 15,
  prisonerPistolChance: 5,
});

export const Weapon = Object.freeze({
  FIST: "fist", PISTOL: "pistol", TASER: "taser", AUTOMATIC: "automatic",
  SNIPER: "sniper", MACHETE: "machete", KNIFE: "knife",
  CROWBAR: "crowbar", BAT: "bat", SHANK: "shank", BATON: "baton"
});

export const NPCType = Object.freeze({
  REGULAR: "regular", SHOOTER: "shooter", GUARD: "guard", BOSS: "boss"
});

export const AIState = Object.freeze({
  PATROLLING: "patrolling", ALERT: "alert", SEARCHING: "searching", COMBAT: "combat"
});

export const AIBehavior = Object.freeze({ AGGRESSOR: "aggressor", SUPPORT: "support" });

export const PlayerSpeeds = Object.freeze({
  WALK: 5, CROUCH: 2, WALK_STEP: .55, RUN_STEP: .35, CROUCH_STEP: .8,
  DODGE_DURATION: .45, DODGE_COOLDOWN: 1.25
});
