#pragma once
#include <SFML/Graphics.hpp>

enum class GameMode { CLASSIC_ACTION, STEALTH_MODE };
enum class WeaponType { FIST, PISTOL, AUTOMATIC, SNIPER };
enum class NPCType { REGULAR, SHOOTER, BOSS };

struct Medkit {
    sf::Vector3f position;
    bool isCollected = false;
};

struct GameSettings {
    // --- Игрок ---
    int playerHealth = 250;
    float playerRunSpeed = 16.0f;
    float healthRegenRate = 5.0f;
    float healthRegenDelay = 8.0f;
    int lowHealthThreshold = 100;
    float playerMaxStamina = 100.0f;
    float staminaDrainRate = 15.0f;
    float staminaRegenRate = 10.0f;
    float staminaRegenDelay = 2.0f;
    int outOfBreathStepThreshold = 30;

    // --- Аптечки ---
    int medkitCount = 5;
    int medkitHealAmount = 75;

    // --- NPC ---
    int regularHealth = 120;
    int shooterHealth = 150;
    int bossHealth = 500;
    bool meleeNpcCanAttack = true;
    float npcWalkSpeed = 1.5f;
    float npcRunSpeed = 14.5f;
    int npcStunChanceOnDamage = 40;
    float npcStunDuration = 0.75f;

    // --- Оружие ---
    int fistDamage = 35;
    int pistolDamage = 40;
    int automaticDamage = 25;
    int sniperDamage = 65;
    float fistVolume = 100.0f;
    float pistolVolume = 110.0f;
    float automaticVolume = 100.0f;
    float sniperVolume = 120.0f;

    // --- Игровой мир ---
    float worldSize = 100.0f;
    int wallCount = 25;
    float gracePeriod = 5.0f;
    float respawnTime = 30.0f;
};
