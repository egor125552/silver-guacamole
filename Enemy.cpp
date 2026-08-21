#include "Enemy.h"
#include "Player.h"
#include "SoundEngine.h"
#include "utils.h"
#include <cmath>
#include <sstream>
#include <algorithm>

namespace {
float planarDistance(const sf::Vector3f& a, const sf::Vector3f& b) {
    return std::hypot(a.x - b.x, a.z - b.z);
}

bool collidesAt(const sf::Vector3f& pos, const std::vector<sf::FloatRect>& walls) {
    sf::FloatRect bounds({pos.x - 0.4f, pos.z - 0.4f}, {0.8f, 0.8f});
    return std::any_of(walls.begin(), walls.end(), [&](const auto& wall) {
        return wall.findIntersection(bounds).has_value();
    });
}
}

Enemy::Enemy(sf::Vector3f startPos, NPCType npcType, const GameSettings& settings) : type(npcType) {
    position = startPos;
    respawn(startPos, settings);
}

void Enemy::move(sf::Vector3f direction, float speed, float deltaTime, const std::vector<sf::FloatRect>& walls) {
    const float length = std::hypot(direction.x, direction.z);
    if (length <= 0.0001f) return;
    direction.x /= length;
    direction.z /= length;

    const sf::Vector3f desired = position + direction * speed * deltaTime;
    if (!collidesAt(desired, walls)) {
        position = desired;
        return;
    }

    const sf::Vector3f slideX{desired.x, position.y, position.z};
    if (!collidesAt(slideX, walls)) {
        position = slideX;
        return;
    }

    const sf::Vector3f slideZ{position.x, position.y, desired.z};
    if (!collidesAt(slideZ, walls)) {
        position = slideZ;
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
        case AIState::SEARCHING: {
            if (stateTimer.getElapsedTime().asSeconds() > 15.0f) {
                state = AIState::PATROLLING;
                setNewRandomTarget(settings);
                decisionClock.restart();
                break;
            }

            sf::Vector3f direction = targetPosition - position;
            float distanceToTarget = std::hypot(direction.x, direction.z);
            if (distanceToTarget > 1.5f) {
                move(direction, walkSpeed * 1.25f, deltaTime, walls);
                if (stepClock.getElapsedTime().asSeconds() > WALK_STEP_INTERVAL) {
                    engine.playSound("footstep", position, 75.f);
                    stepClock.restart();
                }
            } else if (decisionClock.getElapsedTime().asSeconds() > 2.0f) {
                targetPosition.x = position.x + getFloat(-8.0f, 8.0f);
                targetPosition.y = 0.f;
                targetPosition.z = position.z + getFloat(-8.0f, 8.0f);
                decisionClock.restart();
            }
            break;
        }
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
        move(direction, runSpeed, deltaTime, walls);
        if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
            engine.playSound("footstep", position, 90.f);
            stepClock.restart();
        }
    } else if (stateTimer.getElapsedTime().asSeconds() > 4.0f) {
        state = AIState::SEARCHING;
        stateTimer.restart();
        decisionClock.restart();
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
            if (intersection_dist < distance) return false;
        }
    }
    return true;
}

void Enemy::updateCombat(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<Enemy>>& allEnemies) {
    (void)allEnemies;
    if (!player.isAlive) {
        state = AIState::PATROLLING;
        return;
    }

    const float distanceToPlayer = planarDistance(position, player.position);

    if (!hasLineOfSight(player.position, walls)) {
        targetPosition = player.position;
        state = AIState::SEARCHING;
        stateTimer.restart();
        decisionClock.restart();
        return;
    }

    targetPosition = player.position;

    auto playMovementStep = [&]() {
        if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
            engine.playSound("footstep", position, 90.f);
            stepClock.restart();
        }
    };

    if (behavior == AIBehavior::AGGRESSOR) {
        const float meleeAttackRange = 1.8f;
        if (distanceToPlayer > meleeAttackRange) {
            move(player.position - position, runSpeed, deltaTime, walls);
            playMovementStep();
        } else if (settings.meleeNpcCanAttack && lastAttackClock.getElapsedTime().asSeconds() > 1.2f) {
            lastAttackClock.restart();
            int damage = settings.fistDamage;
            std::string sound = "punch";
            if (weapon == WeaponType::KNIFE || weapon == WeaponType::SHANK) {
                damage = weapon == WeaponType::KNIFE ? settings.knifeDamage : settings.shankDamage;
                sound = "Knife_Swish";
            } else if (weapon == WeaponType::BATON || weapon == WeaponType::CROWBAR || weapon == WeaponType::BAT) {
                damage = weapon == WeaponType::BATON ? settings.batonDamage : (weapon == WeaponType::CROWBAR ? settings.crowbarDamage : settings.batDamage);
                sound = weapon == WeaponType::BAT ? "Bat_Swish" : "Blunt_Metal_Swish";
            } else if (weapon == WeaponType::MACHETE) {
                damage = settings.macheteDamage;
                sound = "Machete_Swish";
            }
            engine.playSound(sound, position);
            if (getInt(1, 100) <= 85) player.takeDamage(damage, engine, this);
        }
        return;
    }

    float attackRange = 30.0f;
    float minimumComfortRange = 8.0f;
    float cooldown = 1.5f;
    int hitChance = 60;
    int damage = settings.pistolDamage;
    std::string attackSound = "pistol";
    bool guaranteedStun = false;

    switch (weapon) {
        case WeaponType::TASER:
            attackRange = settings.taserRange;
            minimumComfortRange = 0.0f;
            cooldown = settings.taserCooldown;
            hitChance = 100;
            damage = 0;
            attackSound = "Taser_Fire";
            guaranteedStun = true;
            break;
        case WeaponType::AUTOMATIC:
            attackRange = 25.0f;
            minimumComfortRange = 7.0f;
            cooldown = 0.35f;
            hitChance = 45;
            damage = settings.automaticDamage;
            attackSound = "automatic";
            break;
        case WeaponType::SNIPER:
            attackRange = 65.0f;
            minimumComfortRange = 22.0f;
            cooldown = 2.0f;
            hitChance = 70;
            damage = settings.sniperDamage;
            attackSound = "sniper";
            break;
        case WeaponType::PISTOL:
        default:
            break;
    }

    if (distanceToPlayer > attackRange * 0.92f) {
        move(player.position - position, runSpeed * 0.78f, deltaTime, walls);
        playMovementStep();
        return;
    }

    if (minimumComfortRange > 0.0f && distanceToPlayer < minimumComfortRange) {
        move(position - player.position, runSpeed * 0.58f, deltaTime, walls);
        playMovementStep();
    }

    if (distanceToPlayer <= attackRange && lastAttackClock.getElapsedTime().asSeconds() > cooldown) {
        lastAttackClock.restart();
        engine.playSound(attackSound, position);
        if (getInt(1, 100) <= hitChance) {
            player.takeDamage(damage, engine, this, guaranteedStun);
        }
    }
}

void Enemy::investigate(sf::Vector3f pos) {
    if (state == AIState::PATROLLING || state == AIState::SEARCHING) {
        state = AIState::ALERT;
        targetPosition = pos;
        stateTimer.restart();
        decisionClock.restart();
    }
}

bool Enemy::canRespawn(float respawnTime) const {
    return !isAlive && deathClock.getElapsedTime().asSeconds() > respawnTime;
}

void Enemy::onDeath(SoundEngine& engine) {
    if (deathNotified) return;
    deathNotified = true;
    deathClock.restart();
    engine.onEnemyDied(this);
}

void Enemy::configureLoadout(const GameSettings& settings) {
    behavior = AIBehavior::AGGRESSOR;
    weapon = WeaponType::FIST;

    switch (type) {
        case NPCType::REGULAR: {
            maxHealth = settings.regularHealth;
            const int roll = getInt(1, 100);
            if (roll <= settings.prisonerPistolChance) {
                weapon = WeaponType::PISTOL;
                behavior = AIBehavior::SUPPORT;
            } else {
                const int meleeRoll = getInt(1, 5);
                if (meleeRoll == 1) weapon = WeaponType::KNIFE;
                else if (meleeRoll == 2) weapon = WeaponType::SHANK;
                else if (meleeRoll == 3) weapon = WeaponType::BAT;
                else if (meleeRoll == 4) weapon = WeaponType::CROWBAR;
            }
            break;
        }
        case NPCType::SHOOTER:
            maxHealth = settings.shooterHealth;
            weapon = WeaponType::PISTOL;
            behavior = AIBehavior::SUPPORT;
            break;
        case NPCType::GUARD: {
            maxHealth = settings.shooterHealth;
            const int roll = getInt(1, 100);
            if (roll <= settings.guardPistolChance) {
                weapon = WeaponType::PISTOL;
                behavior = AIBehavior::SUPPORT;
            } else if (roll <= settings.guardPistolChance + settings.guardAutomaticChance) {
                weapon = WeaponType::AUTOMATIC;
                behavior = AIBehavior::SUPPORT;
            } else if (roll <= settings.guardPistolChance + settings.guardAutomaticChance + settings.guardTaserChance) {
                weapon = WeaponType::TASER;
                behavior = AIBehavior::SUPPORT;
            } else {
                weapon = WeaponType::BATON;
                behavior = AIBehavior::AGGRESSOR;
            }
            break;
        }
        case NPCType::BOSS:
            maxHealth = settings.bossHealth;
            weapon = WeaponType::AUTOMATIC;
            behavior = AIBehavior::SUPPORT;
            break;
    }
}

void Enemy::respawn(sf::Vector3f newPosition, const GameSettings& settings) {
    isAlive = true;
    state = AIState::PATROLLING;
    detectionLevel = 0.0f;
    hasReactedToDeath = false;
    isStunned = false;
    deathNotified = false;
    walkSpeed = settings.npcWalkSpeed;
    runSpeed = settings.npcRunSpeed;
    isWaiting = false;
    configureLoadout(settings);
    health = maxHealth;
    position = newPosition;
    setNewRandomTarget(settings);
    decisionClock.restart();
    stateTimer.restart();
    lastAttackClock.restart();
}

bool Enemy::takeDamage(int damage, SoundEngine& engine, Character* attacker, bool guaranteedStun) {
    if (!isAlive) return false;

    engine.playSound("hit", position);
    const bool wasAlive = isAlive;
    const bool result = Character::takeDamage(damage, engine, attacker, guaranteedStun);

    if (result && isAlive) {
        if (guaranteedStun) {
            stunFor(5.f);
            engine.playSound("Stun", position);
            logError("DEBUG_STUN");
        } else if (getInt(1, 100) <= engine.getSettings().npcStunChanceOnDamage) {
            stunFor(engine.getSettings().npcStunDuration);
        }
        if (state != AIState::COMBAT && attacker != nullptr) {
            engine.onEnemySpottedPlayer(this, true);
        }
    }

    if (wasAlive && !isAlive) onDeath(engine);
    return result;
}

void Enemy::setNewRandomTarget(const GameSettings& settings) {
    targetPosition.x = getFloat(-settings.worldSize, settings.worldSize);
    targetPosition.y = 0;
    targetPosition.z = getFloat(-settings.worldSize, settings.worldSize);
}
