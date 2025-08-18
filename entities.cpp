#include "entities.h"
#include "SoundEngine.h"
#include "utils.h"
#include <cmath>
#include <algorithm>
#include <optional>

bool Character::takeDamage(int damage, SoundEngine& engine, Character* attacker) {
    if (!isAlive) return false;
    health -= damage;
    lastDamageTakenClock.restart();
    if (health <= 0) { health = 0; isAlive = false; }
    return true;
}

Player::Player(const GameSettings& settings) { reset(settings); }
void Player::update(float deltaTime, const GameSettings& settings) {
    if (!isAlive) return;
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
bool Player::takeDamage(int damage, SoundEngine& engine, Character* attacker) {
    if (godMode || lastDamageTakenClock.getElapsedTime().asSeconds() < 0.2f) return false;
    healthRegenDelayClock.restart();
    healthRegenBuffer = 0.0f; 
    engine.playSound("hit", {0,0,0}, 100.f, true);
    float healthPercentage = static_cast<float>(health - damage) / maxHealth;
    if (healthPercentage < 0) healthPercentage = 0;
    float pitch = 0.5f + healthPercentage;
    engine.playSound("HealthIndicator", {0,0,0}, 100.f, true, pitch);
    return Character::takeDamage(damage, engine, attacker);
}
void Player::reset(const GameSettings& settings) {
    isAlive = true; godMode = false; isRunning = false; isCrouching = false;
    maxHealth = settings.playerHealth; health = settings.playerHealth; healthRegenBuffer = 0.0f;
    position = {0.f, 0.f, 0.f}; setPosition(position); runSpeed = settings.playerRunSpeed;
    currentWeapon = WeaponType::FIST;
    lastAttackClock.restart(); lastDamageTakenClock.restart(); healthRegenDelayClock.restart();
}
void Player::toggleCrouch() { if (isAlive) { isCrouching = !isCrouching; if (isCrouching) isRunning = false; } }
bool Player::isRegenOnCooldown(const GameSettings& settings) const { return healthRegenDelayClock.getElapsedTime().asSeconds() < settings.healthRegenDelay; }

NPC::NPC(sf::Vector3f startPos, NPCType npcType, const GameSettings& settings) : type(npcType) { position = startPos; respawn(startPos, settings); }

void NPC::move(sf::Vector3f direction, float speed, float deltaTime, const std::vector<sf::FloatRect>& walls) {
    sf::Vector3f newPos = position + direction * speed * deltaTime;
    sf::FloatRect npcBounds({newPos.x - 0.4f, newPos.z - 0.4f}, {0.8f, 0.8f});
    
    bool collision = false;
    for (const auto& wall : walls) {
        if (wall.findIntersection(npcBounds).has_value()) {
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
        for (const auto& wall : walls) if (wall.findIntersection(boundsX).has_value()) { collisionX = true; break; }
        if (!collisionX) {
            position = slideXPos;
            return;
        }
        sf::Vector3f slideZPos = position + sf::Vector3f(0, 0, direction.z) * speed * deltaTime;
        sf::FloatRect boundsZ({slideZPos.x - 0.4f, slideZPos.z - 0.4f}, {0.8f, 0.8f});
        bool collisionZ = false;
        for (const auto& wall : walls) if (wall.findIntersection(boundsZ).has_value()) { collisionZ = true; break; }
        if (!collisionZ) {
            position = slideZPos;
        }
    }
}

void NPC::update(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, GameMode gameMode, const std::vector<std::unique_ptr<NPC>>& allNpcs) {
    if (!isAlive) return;

    // ПРОВЕРКА НА ОГЛУШЕНИЕ: Если NPC оглушен, он ничего не делает.
    if (isStunned) {
        if (stunClock.getElapsedTime().asSeconds() > settings.npcStunDuration) {
            isStunned = false;
        } else {
            return; // Пропускаем всю логику, пока оглушен
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
                engine.playSound("Footstep", position, 70.f); stepClock.restart();
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
            engine.playSound("Footstep", position, 90.f); stepClock.restart();
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

void NPC::updateCombat(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, const std::vector<std::unique_ptr<NPC>>& allNpcs) {
    if (!player.isAlive) {
        state = AIState::PATROLLING; // Если игрок мертв, возвращаемся к патрулированию
        return;
    }

    // Проверяем линию видимости. Если ее нет, пытаемся двигаться к последней известной позиции игрока.
    bool los = hasLineOfSight(player.position, walls);
    if (!los) { 
        // Простое движение в сторону игрока, даже если он за стеной
        sf::Vector3f direction = player.position - position;
        float len = std::hypot(direction.x, direction.z);
        if (len > 1.0f) { // Двигаемся, пока не подойдем достаточно близко
            direction /= len;
            move(direction, runSpeed, deltaTime, walls);
            if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
                engine.playSound("Footstep", position, 100.f);
                stepClock.restart();
            }
        }
        // Можно добавить таймер, чтобы NPC не искал игрока вечно, если тот ушел
        return;
    }

    // Если есть линия видимости, обновляем логику поведения
    targetPosition = player.position; // Запоминаем позицию игрока как цель
    float distanceToPlayer = std::hypot(player.position.x - position.x, player.position.z - position.z);

    // --- ЛОГИКА ДЛЯ РАЗНЫХ ТИПОВ ПОВЕДЕНИЯ ---

    if (behavior == AIBehavior::AGGRESSOR) {
        const float meleeAttackRange = 1.8f;
        const float meleeCooldown = 1.2f;
        bool isMoving = false;

        // ЛОГИКА ДВИЖЕНИЯ: Двигаемся, если не в радиусе атаки
        if (distanceToPlayer > meleeAttackRange) {
            sf::Vector3f direction = player.position - position;
            float len = std::hypot(direction.x, direction.z);
            if (len > 0) {
                direction /= len;
                move(direction, runSpeed, deltaTime, walls);
                isMoving = true;
            }
        }

        // ЛОГИКА АТАКИ: Атакуем, если в радиусе и кулдаун прошел.
        // Эта проверка теперь НЕЗАВИСИМА от движения.
        if (distanceToPlayer <= meleeAttackRange && settings.meleeNpcCanAttack && lastAttackClock.getElapsedTime().asSeconds() > meleeCooldown) {
            lastAttackClock.restart();
            engine.playSound("punch", position, 100.f, false, getFloat(0.8f, 0.9f));
            // Проверка на попадание
            if (getInt(1, 100) <= 90) {
                player.takeDamage(settings.fistDamage, engine, this);
            } else {
                engine.playSound("miss", position, 90.f, false, 1.2f);
            }
        }

        // Звук шагов, если NPC двигался
        if (isMoving && stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
            engine.playSound("Footstep", position, 100.f);
            stepClock.restart();
        }

    } else if (behavior == AIBehavior::SUPPORT) {
        const float idealDistanceMin = 8.0f;
        const float idealDistanceMax = 15.0f;
        const float rangedAttackRange = 30.0f;
        const float rangedCooldown = 1.5f;
        bool isMoving = false;

        // ЛОГИКА ДВИЖЕНИЯ: Поддерживаем идеальную дистанцию
        sf::Vector3f moveDirection = {0,0,0};
        if (distanceToPlayer < idealDistanceMin) { // Слишком близко, отходим
            moveDirection = position - player.position;
            isMoving = true;
        } else if (distanceToPlayer > idealDistanceMax) { // Слишком далеко, подходим
            moveDirection = player.position - position;
            isMoving = true;
        }
        
        if (isMoving) {
            float len = std::hypot(moveDirection.x, moveDirection.z);
            if (len > 0) {
                moveDirection /= len;
                move(moveDirection, runSpeed, deltaTime, walls);
                if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
                    engine.playSound("Footstep", position, 100.f);
                    stepClock.restart();
                }
            }
        }

        // ЛОГИКА АТАКИ: Стреляем, если в радиусе и кулдаун прошел.
        if (distanceToPlayer < rangedAttackRange && lastAttackClock.getElapsedTime().asSeconds() > rangedCooldown) {
            lastAttackClock.restart();
            engine.playSound("pistol", position);
            // Проверка на попадание (у стрелков точность ниже)
            if (getInt(1, 100) <= 60) {
                player.takeDamage(settings.pistolDamage, engine, this);
            }
        }
    }
}

void NPC::investigate(sf::Vector3f pos) { if (state == AIState::PATROLLING) { state = AIState::ALERT; targetPosition = pos; stateTimer.restart(); } }
bool NPC::canRespawn(float respawnTime) const { return !isAlive && deathClock.getElapsedTime().asSeconds() > respawnTime; }
void NPC::onDeath(SoundEngine& engine) { deathClock.restart(); engine.onNpcDied(this); }

void NPC::respawn(sf::Vector3f newPosition, const GameSettings& settings) {
    isAlive = true; state = AIState::PATROLLING; detectionLevel = 0.0f; hasReactedToDeath = false;
    isStunned = false; // Сбрасываем флаг оглушения при респавне
    walkSpeed = settings.npcWalkSpeed; runSpeed = settings.npcRunSpeed; isWaiting = false;
    
    if (getInt(0, 1) == 0) {
        weapon = WeaponType::FIST;
        behavior = AIBehavior::AGGRESSOR;
        maxHealth = settings.regularHealth;
    } else {
        weapon = WeaponType::PISTOL;
        behavior = AIBehavior::SUPPORT;
        maxHealth = settings.shooterHealth;
    }
    
    health = maxHealth; position = newPosition;
    setNewRandomTarget(settings); decisionClock.restart();
}

bool NPC::takeDamage(int damage, SoundEngine& engine, Character* attacker) {
    if (!isAlive || isStunned) return false; // Нельзя нанести урон уже оглушенному NPC
    
    engine.playSound("hit", position);
    float healthPercentage = static_cast<float>(health - damage) / maxHealth;
    if (healthPercentage < 0) healthPercentage = 0;
    float pitch = 0.7f + healthPercentage * 0.6f;
    engine.playSound("HealthIndicator", position, 80.f, false, pitch);
    
    bool result = Character::takeDamage(damage, engine, attacker);
    
    if (result && isAlive) { // Если урон прошел и NPC жив
        // Проверяем шанс наложения оглушения
        if (getInt(1, 100) <= engine.getSettings().npcStunChanceOnDamage) {
            isStunned = true;
            stunClock.restart();
            engine.playSound("Stun", position, 100.f);
        }
        // Переход в режим боя, если еще не в нем
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