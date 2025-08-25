#include "entities.h"
#include "SoundEngine.h"
#include "utils.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <optional>
#include <iostream>

// Forward declaration for the global error logging function from SoundEngine.cpp
void logError(const std::string& message);

void Character::stunFor(float duration) {
    if (duration > 0.f) {
        isStunned = true;
        stunClock.restart();
        currentStunDuration = duration;
    }
}

bool Character::takeDamage(int damage, SoundEngine& engine, Character* attacker, bool guaranteedStun) {
    if (!isAlive) return false;
    health -= damage;
    lastDamageTakenClock.restart();
    if (health <= 0) { health = 0; isAlive = false; }
    return true;
}

Player::Player(const GameSettings& settings) { reset(settings); }
void Player::update(float deltaTime, const GameSettings& settings, const std::vector<std::unique_ptr<NPC>>& npcs) {
    if (isStunned) {
        if (stunClock.getElapsedTime().asSeconds() > currentStunDuration) {
            isStunned = false;
        } else {
            return; // Can't do anything while stunned
        }
    }
    if (!isAlive) return;

    // Dodge timer logic
    if (isDodging && dodgeTimer.getElapsedTime().asSeconds() > 0.5f) { // 0.5 second dodge duration
        isDodging = false;
    }

    // Combat stance logic
    if (inCombatStance) {
        bool enemyNearby = false;
        for (const auto& npc : npcs) {
            if (npc->isAlive && npc->state == AIState::COMBAT) {
                float distance = std::hypot(position.x - npc->position.x, position.z - npc->position.z);
                if (distance < 25.0f) { // 25m check radius
                    enemyNearby = true;
                    timeSinceLastCombatEvent.restart(); // Keep stance active if enemy is near
                    break;
                }
            }
        }

        if (timeSinceLastCombatEvent.getElapsedTime().asSeconds() > 10.0f && !enemyNearby) {
            inCombatStance = false;
        }
    }

    if (health < maxHealth && healthRegenDelayClock.getElapsedTime().asSeconds() > settings.healthRegenDelay) {
        healthRegenBuffer += settings.healthRegenRate * deltaTime;
        if (healthRegenBuffer >= 1.0f) {
            int amountToHeal = static_cast<int>(healthRegenBuffer);
            health = std::min(maxHealth, health + amountToHeal);
            healthRegenBuffer -= amountToHeal;
        }
    }
}
void Player::setPosition(const sf::Vector3f& newPos) { position = newPos; sf::Listener::setPosition(position); }
void Player::switchWeapon(WeaponType newWeapon) { if (isAlive) currentWeapon = newWeapon; }
bool Player::takeDamage(int damage, SoundEngine& engine, Character* attacker, bool guaranteedStun) {
    if (godMode || isDodging || lastDamageTakenClock.getElapsedTime().asSeconds() < 0.2f) return false;

    if (guaranteedStun) {
        stunFor(5.f); // Stun player for 5 seconds
        // We can reuse the NPC stun sound for the player for now
        engine.playSound("Stun", {0,0,0}, 100.f, true);
    }

    healthRegenDelayClock.restart();
    healthRegenBuffer = 0.0f;
    inCombatStance = true;
    timeSinceLastCombatEvent.restart();
    engine.playSound("player_hit", {0,0,0}, 100.f, true);
    float healthPercentage = static_cast<float>(health - damage) / maxHealth;
    if (healthPercentage < 0) healthPercentage = 0;
    float pitch = 0.5f + healthPercentage;
    engine.playSound("HealthIndicator", {0,0,0}, 100.f, true, pitch);
    return Character::takeDamage(damage, engine, attacker);
}
void Player::reset(const GameSettings& settings) {
    isAlive = true; godMode = false; isRunning = false; isCrouching = false; inCombatStance = false; isDodging = false;
    maxHealth = settings.playerHealth; health = settings.playerHealth; healthRegenBuffer = 0.0f;
    position = {0.f, 0.f, 0.f}; setPosition(position); runSpeed = settings.playerRunSpeed;
    currentWeapon = WeaponType::FIST;
    lastAttackClock.restart(); lastDamageTakenClock.restart(); healthRegenDelayClock.restart(); timeSinceLastCombatEvent.restart();
}
void Player::toggleCrouch() { if (isAlive) { isCrouching = !isCrouching; if (isCrouching) isRunning = false; } }

void Player::dodge() {
    if (isAlive && inCombatStance && !isDodging) {
        isDodging = true;
        dodgeTimer.restart();
        // Maybe play a sound here in the future
    }
}
bool Player::isRegenOnCooldown(const GameSettings& settings) const { return healthRegenDelayClock.getElapsedTime().asSeconds() < settings.healthRegenDelay; }

NPC::NPC(sf::Vector3f startPos, NPCType npcType, const GameSettings& settings) : type(npcType) { position = startPos; respawn(startPos, settings); }

// ВЕРСИЯ ДЛЯ SFML 3.0
void NPC::move(sf::Vector3f direction, float speed, float deltaTime, const std::vector<sf::FloatRect>& walls) {
    sf::Vector3f newPos = position + direction * speed * deltaTime;
    sf::FloatRect npcBounds({newPos.x - 0.4f, newPos.z - 0.4f}, {0.8f, 0.8f});

    bool collision = false;
    for (const auto& wall : walls) {
        if (wall.findIntersection(npcBounds)) {
            collision = true;
            break;
        }
    }

    if (!collision) {
        position = newPos;
    } else {
        sf::Vector3f slideXPos = position + sf::Vector3f(direction.x, 0, 0) * speed * deltaTime;
        sf::FloatRect boundsX({slideXPos.x - 0.4f, slideXPos.z - 0.4f}, {0.8f, 0.8f});
        bool collisionX = false;
        for (const auto& wall : walls) if (wall.findIntersection(boundsX)) { collisionX = true; break; }
        if (!collisionX) {
            position = slideXPos;
            return;
        }
        sf::Vector3f slideZPos = position + sf::Vector3f(0, 0, direction.z) * speed * deltaTime;
        sf::FloatRect boundsZ({slideZPos.x - 0.4f, slideZPos.z - 0.4f}, {0.8f, 0.8f});
        bool collisionZ = false;
        for (const auto& wall : walls) if (wall.findIntersection(boundsZ)) { collisionZ = true; break; }
        if (!collisionZ) {
            position = slideZPos;
        }
    }
}

void NPC::update(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, GameMode gameMode, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<NPC>>& allNpcs) {
    if (!isAlive) return;

    if (isStunned) {
        if (stunClock.getElapsedTime().asSeconds() > currentStunDuration) {
            isStunned = false;
        } else {
            return;
        }
    }

    switch (state) {
        case AIState::PATROLLING: updatePatrolling(deltaTime, engine, walls); break;
        case AIState::ALERT:      updateAlert(deltaTime, engine, walls); break;
        case AIState::COMBAT:     updateCombat(deltaTime, player, engine, settings, walls, allNpcs); break;
    }
}

void NPC::updatePatrolling(float deltaTime, SoundEngine& engine, const std::vector<sf::FloatRect>& walls) {
    if (isWaiting) {
        if (stateTimer.getElapsedTime().asSeconds() > getFloat(5.0f, 10.0f)) {
            isWaiting = false; setNewRandomTarget(engine.getSettings()); decisionClock.restart();
        }
    } else {
        sf::Vector3f direction = targetPosition - position;
        float distance = std::hypot(direction.x, direction.z);
        if (distance > 1.0f) {
            isMoving = true;
            direction /= distance;
            move(direction, walkSpeed, deltaTime, walls);
            if (stepClock.getElapsedTime().asSeconds() > WALK_STEP_INTERVAL) {
                engine.playSound("footstep", position, 70.f); stepClock.restart();
            }
        } else { isMoving = false; }
    }
    if (decisionClock.getElapsedTime().asSeconds() > getFloat(15.0f, 20.0f)) {
        isWaiting = true; isMoving = false; stateTimer.restart(); decisionClock.restart();
    }
}

void NPC::updateAlert(float deltaTime, SoundEngine& engine, const std::vector<sf::FloatRect>& walls) {
    sf::Vector3f direction = targetPosition - position;
    float distanceToTarget = std::hypot(direction.x, direction.z);
    if (distanceToTarget > 2.0f) {
        direction /= distanceToTarget;
        move(direction, runSpeed, deltaTime, walls);
        if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
            engine.playSound("footstep", position, 90.f); stepClock.restart();
        }
    } else {
        if (stateTimer.getElapsedTime().asSeconds() > 10.0f) {
            state = AIState::PATROLLING; setNewRandomTarget(engine.getSettings());
        }
    }
}

bool NPC::hasLineOfSight(const sf::Vector3f& target, const std::vector<sf::FloatRect>& walls) {
    sf::Vector2f start(position.x, position.z);
    sf::Vector2f end(target.x, target.z);
    sf::Vector2f dir = end - start;
    float distance = std::hypot(dir.x, dir.y);
    if (distance > 0) dir /= distance;

    for(const auto& wall : walls) {
        sf::Vector2f intersection_point = rayIntersectsRect(start, dir, wall);
        if (intersection_point.x != -1) {
            float intersection_dist = std::hypot(intersection_point.x - start.x, intersection_point.y - start.y);
            if (intersection_dist < distance) {
                return false;
            }
        }
    }
    return true;
}

void NPC::updateCombat(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<NPC>>& allNpcs) {
    if (!player.isAlive) {
        state = AIState::PATROLLING;
        return;
    }

    bool los = hasLineOfSight(player.position, walls);
    if (!los) {
        sf::Vector3f direction = player.position - position;
        float len = std::hypot(direction.x, direction.z);
        if (len > 1.0f) {
            direction /= len;
            move(direction, runSpeed, deltaTime, walls);
            if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
                engine.playSound("footstep", position, 100.f);
                stepClock.restart();
            }
        }
        return;
    }

    targetPosition = player.position;
    float distanceToPlayer = std::hypot(player.position.x - position.x, player.position.z - position.z);

    if (behavior == AIBehavior::AGGRESSOR) {
        const float meleeAttackRange = 1.8f;
        const float meleeCooldown = 1.2f;
        bool isMoving = false;

        if (distanceToPlayer > meleeAttackRange) {
            sf::Vector3f direction = player.position - position;
            float len = std::hypot(direction.x, direction.z);
            if (len > 0) {
                direction /= len;
                move(direction, runSpeed, deltaTime, walls);
                isMoving = true;
            }
        }

        if (distanceToPlayer <= meleeAttackRange && settings.meleeNpcCanAttack && lastAttackClock.getElapsedTime().asSeconds() > meleeCooldown) {
            lastAttackClock.restart();
            engine.playSound("punch", position, 100.f, false, getFloat(0.8f, 0.9f));
            if (getInt(1, 100) <= 90) {
                player.takeDamage(settings.fistDamage, engine, this);
            } else {
                engine.playSound("miss", position, 90.f, false, 1.2f);
            }
        }

        if (isMoving && stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
            engine.playSound("footstep", position, 100.f);
            stepClock.restart();
        }

    } else if (behavior == AIBehavior::SUPPORT) {
        const float idealDistanceMin = 8.0f;
        const float idealDistanceMax = 15.0f;
        const float rangedAttackRange = 30.0f;
        const float rangedCooldown = 1.5f;
        bool isMoving = false;

        sf::Vector3f moveDirection = {0,0,0};
        if (distanceToPlayer < idealDistanceMin) {
            moveDirection = position - player.position;
            isMoving = true;
        } else if (distanceToPlayer > idealDistanceMax) {
            moveDirection = player.position - position;
            isMoving = true;
        }

        if (isMoving) {
            float len = std::hypot(moveDirection.x, moveDirection.z);
            if (len > 0) {
                moveDirection /= len;
                move(moveDirection, runSpeed, deltaTime, walls);
                if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
                    engine.playSound("footstep", position, 100.f);
                    stepClock.restart();
                }
            }
        }

        if (weapon == WeaponType::TASER) {
            if (distanceToPlayer < settings.taserRange && lastAttackClock.getElapsedTime().asSeconds() > settings.taserCooldown) {
                lastAttackClock.restart();
                engine.playSound("sniper", position); // Placeholder sound
                player.takeDamage(0, engine, this, true); // 0 damage, guaranteed stun
            }
        } else { // For other ranged weapons like PISTOL
            if (distanceToPlayer < rangedAttackRange && lastAttackClock.getElapsedTime().asSeconds() > rangedCooldown) {
                lastAttackClock.restart();
                engine.playSound("pistol", position);
                if (getInt(1, 100) <= 60) {
                    player.takeDamage(settings.pistolDamage, engine, this);
                }
            }
        }
    }
}

void NPC::investigate(sf::Vector3f pos) { if (state == AIState::PATROLLING) { state = AIState::ALERT; targetPosition = pos; stateTimer.restart(); } }
bool NPC::canRespawn(float respawnTime) const { return !isAlive && deathClock.getElapsedTime().asSeconds() > respawnTime; }
void NPC::onDeath(SoundEngine& engine) { deathClock.restart(); engine.onNpcDied(this); }

void NPC::respawn(sf::Vector3f newPosition, const GameSettings& settings) {
    isAlive = true; state = AIState::PATROLLING; detectionLevel = 0.0f; hasReactedToDeath = false;
    isStunned = false;
    walkSpeed = settings.npcWalkSpeed; runSpeed = settings.npcRunSpeed; isWaiting = false;

    if (type == NPCType::REGULAR) {
        const std::vector<WeaponType> prisonerWeapons = {
            WeaponType::FIST, WeaponType::SHANK, WeaponType::KNIFE, WeaponType::CROWBAR, WeaponType::BAT
        };
        weapon = prisonerWeapons[getInt(0, prisonerWeapons.size() - 1)];
        behavior = AIBehavior::AGGRESSOR;
        maxHealth = settings.regularHealth;
    } else if (type == NPCType::GUARD) {
        const std::vector<WeaponType> guardWeapons = {
            WeaponType::BATON, WeaponType::PISTOL, WeaponType::TASER
        };
        weapon = guardWeapons[getInt(0, guardWeapons.size() - 1)];
        // Behavior depends on weapon
        if (weapon == WeaponType::BATON) {
            behavior = AIBehavior::AGGRESSOR;
        } else {
            behavior = AIBehavior::SUPPORT;
        }
        maxHealth = settings.shooterHealth; // Guards are tougher
    } else { // Fallback for other types like SHOOTER or BOSS for now
        weapon = WeaponType::PISTOL;
        behavior = AIBehavior::SUPPORT;
        maxHealth = settings.shooterHealth;
    }

    health = maxHealth; position = newPosition;
    setNewRandomTarget(settings); decisionClock.restart();
}

bool NPC::takeDamage(int damage, SoundEngine& engine, Character* attacker, bool guaranteedStun) {
    if (!isAlive || isStunned) return false;

    engine.playSound("hit", position);
    float healthPercentage = static_cast<float>(health - damage) / maxHealth;
    if (healthPercentage < 0) healthPercentage = 0;
    float pitch = 0.7f + healthPercentage * 0.6f;
    engine.playSound("HealthIndicator", position, 80.f, false, pitch);

    bool result = Character::takeDamage(damage, engine, attacker, guaranteedStun);

    if (result && isAlive) {
        if (guaranteedStun) {
            stunFor(5.f); // Taser stun is 5 seconds
            engine.playSound("Stun", position, 100.f);
            logError("DEBUG_STUN"); // Log message for the test script
        } else if (getInt(1, 100) <= engine.getSettings().npcStunChanceOnDamage) {
            stunFor(engine.getSettings().npcStunDuration); // Regular damage stun
            engine.playSound("Stun", position, 100.f);
        }
        if (state != AIState::COMBAT) {
            engine.onNpcSpottedPlayer(this, true);
        }
    }
    return result;
}

void NPC::setNewRandomTarget(const GameSettings& settings) {
    targetPosition.x = getFloat(-settings.worldSize, settings.worldSize);
    targetPosition.y = 0;
    targetPosition.z = getFloat(-settings.worldSize, settings.worldSize);
}