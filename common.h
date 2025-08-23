#pragma once
#include <SFML/Graphics.hpp>

enum class GameMode { CLASSIC_ACTION, STEALTH_MODE };
enum class WeaponType { FIST, PISTOL, AUTOMATIC, SNIPER };
enum class NPCType { REGULAR, SHOOTER, BOSS };

struct GameSettings {
    // Player Stats
    int playerHealth = 250;
    float playerRunSpeed = 16.0f;
    float healthRegenRate = 5.0f;
    float healthRegenDelay = 8.0f;
    int lowHealthThreshold = 100;
    float playerMaxStamina = 100.0f;
    float staminaDrainRate = 20.0f;
    float staminaRegenRate = 25.0f;

    // NPC Stats
    int regularHealth = 120;
    int shooterHealth = 150;
    int bossHealth = 500;
    bool meleeNpcCanAttack = true;
    float npcWalkSpeed = 1.5f;
    float npcRunSpeed = 14.5f;
    int npcStunChanceOnDamage = 40;
    float npcStunDuration = 0.75f;

    // Weapon Stats
    int fistDamage = 35;
    int pistolDamage = 40;
    int automaticDamage = 25;
    int sniperDamage = 65;
    float fistVolume = 100.0f;
    float pistolVolume = 110.0f;
    float automaticVolume = 100.0f;
    float sniperVolume = 120.0f;
    float fistCooldown = 0.5f;
    float pistolCooldown = 0.4f;

    // World Settings
    float worldSize = 100.0f;
    int wallCount = 25;
    float gracePeriod = 5.0f;
    float respawnTime = 30.0f;
};
