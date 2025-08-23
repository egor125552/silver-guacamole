#pragma once

#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <memory>
#include "common.h"
#include "ai_definitions.h"

class SoundEngine;

enum class AIBehavior {
    AGGRESSOR,
    SUPPORT
};

class Character {
public:
    int health = 100;
    int maxHealth = 100;
    bool isAlive = true;
    sf::Vector3f position;
    sf::Clock lastDamageTakenClock;
    sf::Clock lastAttackClock;

    virtual ~Character() = default;

    virtual bool takeDamage(int damage, SoundEngine& engine, Character* attacker = nullptr);
};

class Player : public Character {
public:
    static constexpr float WALK_SPEED = 5.0f;
    static constexpr float CROUCH_SPEED = 2.0f;
    static constexpr float WALK_STEP_INTERVAL = 0.55f;
    static constexpr float RUN_STEP_INTERVAL = 0.35f;
    static constexpr float CROUCH_STEP_INTERVAL = 0.8f;

    float runSpeed = 18.0f;
    bool isRunning = false;
    bool godMode = false;
    bool isCrouching = false;
    WeaponType currentWeapon = WeaponType::FIST;

    // Stamina
    float maxStamina;
    float currentStamina;

private:
    float healthRegenBuffer = 0.0f;
    sf::Clock healthRegenDelayClock;

public:
    explicit Player(const GameSettings& settings);
    void update(float deltaTime, const GameSettings& settings);
    void setPosition(const sf::Vector3f& newPos);
    void switchWeapon(WeaponType newWeapon);
    void reset(const GameSettings& settings);
    bool takeDamage(int damage, SoundEngine& engine, Character* attacker = nullptr) override;
    void toggleCrouch();
    bool isRegenOnCooldown(const GameSettings& settings) const;
};

class NPC : public Character {
public:
    float walkSpeed = 1.5f;
    float runSpeed = 4.5f;
    static constexpr float WALK_STEP_INTERVAL = 0.6f;
    static constexpr float RUN_STEP_INTERVAL = 0.4f;

    NPCType type;
    WeaponType weapon = WeaponType::FIST;
    AIBehavior behavior = AIBehavior::AGGRESSOR;
    AIState state = AIState::PATROLLING;
    float detectionLevel = 0.0f;
    bool hasReactedToDeath = false;
    bool isStunned = false; // НОВОЕ: Флаг оглушения

private:
    sf::Vector3f targetPosition;
    sf::Clock decisionClock;
    sf::Clock stateTimer;
    sf::Clock stepClock;
    sf::Clock deathClock;
    sf::Clock stunClock; // НОВОЕ: Таймер для отслеживания длительности оглушения
    bool isMoving = false;
    bool isWaiting = false;

    void setNewRandomTarget(const GameSettings& settings);
    void move(sf::Vector3f direction, float speed, float deltaTime, const std::vector<sf::FloatRect>& walls);
    bool hasLineOfSight(const sf::Vector3f& target, const std::vector<sf::FloatRect>& walls);
    void updatePatrolling(float deltaTime, SoundEngine& engine, const std::vector<sf::FloatRect>& walls);
    void updateAlert(float deltaTime, SoundEngine& engine, const std::vector<sf::FloatRect>& walls);
    void updateCombat(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<NPC>>& allNpcs);

public:
    NPC(sf::Vector3f startPos, NPCType npcType, const GameSettings& settings);
    void update(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, GameMode gameMode, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<NPC>>& allNpcs);
    bool canRespawn(float respawnTime) const;
    void respawn(sf::Vector3f newPosition, const GameSettings& settings);
    void onDeath(SoundEngine& engine);
    void investigate(sf::Vector3f pos);
    bool takeDamage(int damage, SoundEngine& engine, Character* attacker) override;
};
