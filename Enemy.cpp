#include "Enemy.h"
#include "Player.h"
#include "SoundEngine.h"
#include "utils.h"
#include <cmath>
#include <sstream>
#include <algorithm>

Enemy::Enemy(sf::Vector3f startPos, NPCType npcType, const GameSettings& settings) : type(npcType) {
    position = startPos;
    respawn(startPos, settings);
}

void Enemy::move(sf::Vector3f direction, float speed, float deltaTime, const std::vector<sf::FloatRect>& walls) {
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
    }
}

void Enemy::update(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, GameMode gameMode, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<Enemy>>& allEnemies) {
    if (!isAlive || isStunned) {
        if (isStunned && stunClock.getElapsedTime().asSeconds() > currentStunDuration) {
            isStunned = false;
        }
        return;
    }

    switch (state) {
        case AIState::PATROLLING:
            updatePatrolling(deltaTime, engine, walls);
            break;
        case AIState::ALERT:
            updateAlert(deltaTime, engine, walls);
            break;
        case AIState::SEARCHING:
            // В состоянии поиска враг просто ждет некоторое время,
            // оглядываясь по сторонам (пока что просто ждет).
            // Если таймер истек, он возвращается к патрулированию.
            if (stateTimer.getElapsedTime().asSeconds() > 15.0f) { // 15 секунд поиска
                state = AIState::PATROLLING;
                setNewRandomTarget(settings);
            }
            break;
        case AIState::COMBAT:
            updateCombat(deltaTime, player, engine, settings, walls, allEnemies);
            break;
    }
}

void Enemy::updatePatrolling(float deltaTime, SoundEngine& engine, const std::vector<sf::FloatRect>& walls) {
    if (isWaiting) {
        if (stateTimer.getElapsedTime().asSeconds() > getFloat(5.0f, 10.0f)) {
            isWaiting = false;
            setNewRandomTarget(engine.getSettings());
            decisionClock.restart();
        }
    } else {
        sf::Vector3f direction = targetPosition - position;
        float distance = std::hypot(direction.x, direction.z);
        if (distance > 1.0f) {
            isMoving = true;
            direction /= distance;
            move(direction, walkSpeed, deltaTime, walls);
            if (stepClock.getElapsedTime().asSeconds() > WALK_STEP_INTERVAL) {
                engine.playSound("footstep", position, 70.f);
                stepClock.restart();
            }
        } else {
            isMoving = false;
        }
    }
    if (decisionClock.getElapsedTime().asSeconds() > getFloat(15.0f, 20.0f)) {
        isWaiting = true;
        isMoving = false;
        stateTimer.restart();
        decisionClock.restart();
    }
}

void Enemy::updateAlert(float deltaTime, SoundEngine& engine, const std::vector<sf::FloatRect>& walls) {
    sf::Vector3f direction = targetPosition - position;
    float distanceToTarget = std::hypot(direction.x, direction.z);
    if (distanceToTarget > 2.0f) {
        direction /= distanceToTarget;
        move(direction, runSpeed, deltaTime, walls);
        if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
            engine.playSound("footstep", position, 90.f);
            stepClock.restart();
        }
    } else {
        if (stateTimer.getElapsedTime().asSeconds() > 10.0f) {
            state = AIState::PATROLLING;
            setNewRandomTarget(engine.getSettings());
        }
    }
}

bool Enemy::hasLineOfSight(const sf::Vector3f& target, const std::vector<sf::FloatRect>& walls) {
    sf::Vector2f start(position.x, position.z);
    sf::Vector2f end(target.x, target.z);
    sf::Vector2f dir = end - start;
    float distance = std::hypot(dir.x, dir.y);
    if (distance > 0) dir /= distance;

    for (const auto& wall : walls) {
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

void Enemy::updateCombat(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<Enemy>>& allEnemies) {
    if (!player.isAlive) {
        state = AIState::PATROLLING;
        return;
    }

    targetPosition = player.position;
    float distanceToPlayer = std::hypot(player.position.x - position.x, player.position.z - position.z);

    if (!hasLineOfSight(player.position, walls)) {
        // Логика погони, если игрок скрылся из виду
        sf::Vector3f direction = player.position - position;
        move(direction, runSpeed, deltaTime, walls);
        return;
    }

    // Логика атаки в зависимости от поведения
    if (behavior == AIBehavior::AGGRESSOR) {
        const float meleeAttackRange = 1.8f;
        if (distanceToPlayer > meleeAttackRange) {
             sf::Vector3f direction = player.position - position;
             move(direction, runSpeed, deltaTime, walls);
             if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
                engine.playSound("footstep", position, 90.f);
                stepClock.restart();
            }
        } else if (lastAttackClock.getElapsedTime().asSeconds() > 1.2f) {
            lastAttackClock.restart();
            engine.playSound("punch", position);
            if (getInt(1, 100) <= 90) player.takeDamage(settings.fistDamage, engine, this);
        }
    } else if (behavior == AIBehavior::SUPPORT) {
        // Логика для стрелков и электрошокера
        if (weapon == WeaponType::TASER && distanceToPlayer < settings.taserRange && lastAttackClock.getElapsedTime().asSeconds() > settings.taserCooldown) {
            lastAttackClock.restart();
            engine.playSound("sniper", position); // Placeholder
            player.takeDamage(0, engine, this, true); // Оглушение
        } else if (weapon == WeaponType::PISTOL && distanceToPlayer < 30.0f && lastAttackClock.getElapsedTime().asSeconds() > 1.5f) {
            lastAttackClock.restart();
            engine.playSound("pistol", position);
            if (getInt(1, 100) <= 60) player.takeDamage(settings.pistolDamage, engine, this);
        }
    }
}

void Enemy::investigate(sf::Vector3f pos) {
    if (state == AIState::PATROLLING) {
        state = AIState::ALERT;
        targetPosition = pos;
        stateTimer.restart();
    }
}

bool Enemy::canRespawn(float respawnTime) const {
    return !isAlive && deathClock.getElapsedTime().asSeconds() > respawnTime;
}

void Enemy::onDeath(SoundEngine& engine) {
    deathClock.restart();
    // TODO: engine.onEnemyDied(this);
}

void Enemy::respawn(sf::Vector3f newPosition, const GameSettings& settings) {
    isAlive = true; state = AIState::PATROLLING; detectionLevel = 0.0f; hasReactedToDeath = false;
    isStunned = false;
    walkSpeed = settings.npcWalkSpeed; runSpeed = settings.npcRunSpeed; isWaiting = false;
    health = maxHealth; position = newPosition;
    setNewRandomTarget(settings);
    decisionClock.restart();
    // ... остальная логика респавна оружия ...
}

bool Enemy::takeDamage(int damage, SoundEngine& engine, Character* attacker, bool guaranteedStun) {
    if (!isAlive) return false;

    engine.playSound("hit", position);
    bool result = Character::takeDamage(damage, engine, attacker, guaranteedStun);

    if (result && isAlive) {
        if (guaranteedStun) {
            stunFor(5.f);
            engine.playSound("Stun", position);
            logError("DEBUG_STUN"); // Log message for the test script
        }
        if (state != AIState::COMBAT) {
            // TODO: engine.onEnemySpottedPlayer(this, true);
        }
    }
    return result;
}

void Enemy::setNewRandomTarget(const GameSettings& settings) {
    targetPosition.x = getFloat(-settings.worldSize, settings.worldSize);
    targetPosition.y = 0;
    targetPosition.z = getFloat(-settings.worldSize, settings.worldSize);
}
