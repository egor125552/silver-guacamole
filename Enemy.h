#pragma once

#include "Character.h"
#include "ai_definitions.h"
#include <vector>
#include <memory>

// Forward declarations
class Player;
class SoundEngine;

class Enemy : public Character {
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

private:
    sf::Vector3f targetPosition;
    sf::Clock decisionClock;
    sf::Clock stateTimer;
    sf::Clock stepClock;
    sf::Clock deathClock;
    bool isMoving = false;
    bool isWaiting = false;

    void setNewRandomTarget(const GameSettings& settings);
    void move(sf::Vector3f direction, float speed, float deltaTime, const std::vector<sf::FloatRect>& walls);

public:
    // Public for testing purposes
    void updatePatrolling(float deltaTime, SoundEngine& engine, const std::vector<sf::FloatRect>& walls);
    void updateAlert(float deltaTime, SoundEngine& engine, const std::vector<sf::FloatRect>& walls);
    void updateCombat(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<Enemy>>& allEnemies);

    Enemy(sf::Vector3f startPos, NPCType npcType, const GameSettings& settings);

    // The main update function, delegates to state-specific updates
    void update(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, GameMode gameMode, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<Enemy>>& allEnemies);

    bool hasLineOfSight(const sf::Vector3f& target, const std::vector<sf::FloatRect>& walls);
    bool canRespawn(float respawnTime) const;
    void respawn(sf::Vector3f newPosition, const GameSettings& settings);
    void onDeath(SoundEngine& engine);
    void investigate(sf::Vector3f pos);
    bool takeDamage(int damage, SoundEngine& engine, Character* attacker, bool guaranteedStun = false) override;
};
