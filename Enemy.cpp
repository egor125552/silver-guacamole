#include "Enemy.h"
#include "Player.h"
#include "SoundEngine.h"
#include "utils.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <array>

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

sf::Vector3f normalizedPlanar(sf::Vector3f value) {
    const float length = std::hypot(value.x, value.z);
    if (length <= 0.0001f) return {0.f, 0.f, 0.f};
    value.x /= length;
    value.y = 0.f;
    value.z /= length;
    return value;
}

sf::Vector3f rotatePlanar(const sf::Vector3f& direction, float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return {
        direction.x * c - direction.z * s,
        0.f,
        direction.x * s + direction.z * c
    };
}

sf::Vector3f separationFromAllies(
    const Enemy* self,
    const std::vector<std::unique_ptr<Enemy>>& allEnemies,
    float radius = 2.8f)
{
    sf::Vector3f separation{0.f, 0.f, 0.f};
    for (const auto& other : allEnemies) {
        if (!other || other.get() == self || !other->isAlive) continue;
        const float distance = planarDistance(self->position, other->position);
        if (distance <= 0.001f || distance >= radius) continue;
        const float strength = (radius - distance) / radius;
        separation += normalizedPlanar(self->position - other->position) * strength;
    }
    return separation;
}

float flankSignFor(const Enemy& self, const Player& player, const std::vector<std::unique_ptr<Enemy>>& allEnemies) {
    const sf::Vector3f toPlayer = normalizedPlanar(player.position - self.position);
    float balance = 0.f;
    for (const auto& other : allEnemies) {
        if (!other || other.get() == &self || !other->isAlive || other->state != AIState::COMBAT) continue;
        if (planarDistance(self.position, other->position) > 14.f) continue;
        const sf::Vector3f toOther = other->position - self.position;
        balance += toPlayer.x * toOther.z - toPlayer.z * toOther.x;
    }

    if (std::abs(balance) > 0.1f) return balance > 0.f ? -1.f : 1.f;
    // Stable fallback so a group does not all choose exactly the same side.
    return std::sin(self.position.x * 0.37f + self.position.z * 0.19f) >= 0.f ? 1.f : -1.f;
}
}

Enemy::Enemy(sf::Vector3f startPos, NPCType npcType, const GameSettings& settings) : type(npcType) {
    position = startPos;
    respawn(startPos, settings);
}

void Enemy::move(sf::Vector3f direction, float speed, float deltaTime, const std::vector<sf::FloatRect>& walls) {
    direction = normalizedPlanar(direction);
    if (std::hypot(direction.x, direction.z) <= 0.0001f) return;

    const float step = speed * deltaTime;
    const std::array<float, 9> steeringAngles = {
        0.f,
        0.42f, -0.42f,
        0.82f, -0.82f,
        1.20f, -1.20f,
        1.57f, -1.57f
    };

    // Local obstacle avoidance: try the intended direction first, then progressively
    // wider side routes. This is deliberately lightweight but prevents the old behaviour
    // where an NPC simply pressed its face into a wall forever.
    for (float angle : steeringAngles) {
        const sf::Vector3f candidateDirection = rotatePlanar(direction, angle);
        const sf::Vector3f candidate = position + candidateDirection * step;
        if (!collidesAt(candidate, walls)) {
            position = candidate;
            return;
        }
    }
}

void Enemy::update(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, GameMode gameMode, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<Enemy>>& allEnemies) {
    (void)gameMode;
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
            const float searchTime = stateTimer.getElapsedTime().asSeconds();
            if (searchTime > 18.0f) {
                state = AIState::PATROLLING;
                setNewRandomTarget(settings);
                decisionClock.restart();
                break;
            }

            sf::Vector3f direction = targetPosition - position;
            const float distanceToTarget = std::hypot(direction.x, direction.z);
            if (distanceToTarget > 1.2f) {
                move(direction, walkSpeed * 1.35f, deltaTime, walls);
                if (stepClock.getElapsedTime().asSeconds() > WALK_STEP_INTERVAL) {
                    engine.playSound("footstep", position, 75.f);
                    stepClock.restart();
                }
            } else if (decisionClock.getElapsedTime().asSeconds() > 1.15f) {
                // Search an expanding area around the last known position. Candidate points
                // inside walls are rejected so the search itself cannot get stuck on bad RNG.
                const float radius = std::min(14.f, 4.f + searchTime * 0.65f);
                bool found = false;
                for (int attempt = 0; attempt < 12; ++attempt) {
                    sf::Vector3f candidate{
                        position.x + getFloat(-radius, radius),
                        0.f,
                        position.z + getFloat(-radius, radius)
                    };
                    candidate.x = std::clamp(candidate.x, -settings.worldSize + 1.f, settings.worldSize - 1.f);
                    candidate.z = std::clamp(candidate.z, -settings.worldSize + 1.f, settings.worldSize - 1.f);
                    if (!collidesAt(candidate, walls)) {
                        targetPosition = candidate;
                        found = true;
                        break;
                    }
                }
                if (!found) setNewRandomTarget(settings);
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
        if (stateTimer.getElapsedTime().asSeconds() > getFloat(2.5f, 5.5f)) {
            isWaiting = false;
            setNewRandomTarget(engine.getSettings());
            decisionClock.restart();
        }
    } else {
        sf::Vector3f direction = targetPosition - position;
        const float distance = std::hypot(direction.x, direction.z);
        if (distance > 1.0f) {
            isMoving = true;
            move(direction, walkSpeed, deltaTime, walls);
            if (stepClock.getElapsedTime().asSeconds() > WALK_STEP_INTERVAL) {
                engine.playSound("footstep", position, 70.f);
                stepClock.restart();
            }
        } else {
            isMoving = false;
            isWaiting = true;
            stateTimer.restart();
        }
    }

    // A patrol target can still be awkward because the old maps are random. Re-roll it
    // periodically rather than letting an NPC spend half a minute fighting one corner.
    if (decisionClock.getElapsedTime().asSeconds() > 11.0f) {
        isWaiting = false;
        setNewRandomTarget(engine.getSettings());
        decisionClock.restart();
    }
}

void Enemy::updateAlert(float deltaTime, SoundEngine& engine, const std::vector<sf::FloatRect>& walls) {
    sf::Vector3f direction = targetPosition - position;
    const float distanceToTarget = std::hypot(direction.x, direction.z);
    if (distanceToTarget > 1.5f) {
        move(direction, runSpeed * 0.92f, deltaTime, walls);
        if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
            engine.playSound("footstep", position, 90.f);
            stepClock.restart();
        }
    } else if (stateTimer.getElapsedTime().asSeconds() > 1.2f) {
        state = AIState::SEARCHING;
        stateTimer.restart();
        decisionClock.restart();
    }
}

bool Enemy::hasLineOfSight(const sf::Vector3f& target, const std::vector<sf::FloatRect>& walls) {
    sf::Vector2f start(position.x, position.z);
    sf::Vector2f end(target.x, target.z);
    sf::Vector2f dir = end - start;
    const float distance = std::hypot(dir.x, dir.y);
    if (distance > 0) dir /= distance;

    for (const auto& wall : walls) {
        const sf::Vector2f intersectionPoint = rayIntersectsRect(start, dir, wall);
        if (intersectionPoint.x != -1) {
            const float intersectionDistance = std::hypot(intersectionPoint.x - start.x, intersectionPoint.y - start.y);
            if (intersectionDistance < distance) return false;
        }
    }
    return true;
}

void Enemy::updateCombat(float deltaTime, Player& player, SoundEngine& engine, const GameSettings& settings, const std::vector<sf::FloatRect>& walls, const std::vector<std::unique_ptr<Enemy>>& allEnemies) {
    if (!player.isAlive) {
        state = AIState::PATROLLING;
        return;
    }

    const float distanceToPlayer = planarDistance(position, player.position);

    if (!hasLineOfSight(player.position, walls)) {
        // Do not instantly forget the player behind a corner. Run to the last place where
        // they were seen, then perform a real search from there.
        targetPosition = player.position;
        state = AIState::ALERT;
        stateTimer.restart();
        decisionClock.restart();
        return;
    }

    targetPosition = player.position;
    const sf::Vector3f separation = separationFromAllies(this, allEnemies);
    const float flankSign = flankSignFor(*this, player, allEnemies);
    const sf::Vector3f toPlayer = normalizedPlanar(player.position - position);
    const sf::Vector3f perpendicular{-toPlayer.z * flankSign, 0.f, toPlayer.x * flankSign};

    auto playMovementStep = [&]() {
        if (stepClock.getElapsedTime().asSeconds() > RUN_STEP_INTERVAL) {
            engine.playSound("footstep", position, 90.f);
            stepClock.restart();
        }
    };

    if (behavior == AIBehavior::AGGRESSOR) {
        const float meleeAttackRange = 1.8f;
        if (distanceToPlayer > meleeAttackRange) {
            sf::Vector3f pursuit = toPlayer + separation * 1.35f;
            // Once several enemies are fighting, melee NPCs fan out instead of forming one
            // perfectly overlapping line behind the first attacker.
            if (distanceToPlayer > 3.0f) pursuit += perpendicular * 0.48f;
            move(pursuit, runSpeed, deltaTime, walls);
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

    if (distanceToPlayer > attackRange * 0.90f) {
        move(toPlayer + separation * 1.1f + perpendicular * 0.18f, runSpeed * 0.80f, deltaTime, walls);
        playMovementStep();
        return;
    }

    if (minimumComfortRange > 0.0f && distanceToPlayer < minimumComfortRange) {
        move((position - player.position) + separation * 1.4f + perpendicular * 0.35f, runSpeed * 0.62f, deltaTime, walls);
        playMovementStep();
    } else if (weapon != WeaponType::TASER && distanceToPlayer < attackRange * 0.88f) {
        // Ranged enemies no longer freeze in their ideal band. They strafe and spread out,
        // which makes their stereo position change and makes a stationary firing solution harder.
        move(perpendicular + separation * 1.6f, runSpeed * 0.42f, deltaTime, walls);
        playMovementStep();
    }

    if (distanceToPlayer <= attackRange && lastAttackClock.getElapsedTime().asSeconds() > cooldown) {
        lastAttackClock.restart();
        engine.playSound(attackSound, position);

        int adjustedHitChance = hitChance;
        if (player.isRunning) adjustedHitChance -= 8;
        if (distanceToPlayer > attackRange * 0.75f) adjustedHitChance -= 6;
        if (weapon == WeaponType::SNIPER && distanceToPlayer > minimumComfortRange) adjustedHitChance += 5;
        adjustedHitChance = std::clamp(adjustedHitChance, 20, 100);

        if (getInt(1, 100) <= adjustedHitChance) {
            player.takeDamage(damage, engine, this, guaranteedStun);
        }
    }
}

void Enemy::investigate(sf::Vector3f pos) {
    if (state == AIState::COMBAT) return;
    state = AIState::ALERT;
    targetPosition = pos;
    stateTimer.restart();
    decisionClock.restart();
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
