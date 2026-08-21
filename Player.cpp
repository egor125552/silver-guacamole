#include "Player.h"
#include "SoundEngine.h"
#include "Enemy.h"
#include <algorithm>
#include <cmath>
#include <AL/al.h>

Player::Player(const GameSettings& settings) {
    reset(settings);
}

void Player::update(float deltaTime, const GameSettings& settings, const std::vector<std::unique_ptr<Enemy>>& enemies) {
    if (isStunned) {
        if (stunClock.getElapsedTime().asSeconds() > currentStunDuration) {
            isStunned = false;
        } else {
            return;
        }
    }
    if (!isAlive) return;

    if (isDodging && dodgeTimer.getElapsedTime().asSeconds() > DODGE_DURATION) {
        isDodging = false;
    }

    if (inCombatStance) {
        bool enemyNearby = false;
        for (const auto& enemy : enemies) {
            if (enemy->isAlive && enemy->state == AIState::COMBAT) {
                float distance = std::hypot(position.x - enemy->position.x, position.z - enemy->position.z);
                if (distance < 25.0f) {
                    enemyNearby = true;
                    timeSinceLastCombatEvent.restart();
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

void Player::setPosition(const sf::Vector3f& newPos) {
    position = newPos;
    alListener3f(AL_POSITION, position.x, position.y, position.z);
}

void Player::switchWeapon(WeaponType newWeapon) {
    if (isAlive) currentWeapon = newWeapon;
}

bool Player::takeDamage(int damage, SoundEngine& engine, Character* attacker, bool guaranteedStun) {
    if (godMode || isDodging || lastDamageTakenClock.getElapsedTime().asSeconds() < 0.2f) return false;

    if (guaranteedStun) {
        stunFor(5.f);
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
    isAlive = true;
    godMode = false;
    isRunning = false;
    isCrouching = false;
    inCombatStance = false;
    isDodging = false;
    maxHealth = settings.playerHealth;
    health = settings.playerHealth;
    healthRegenBuffer = 0.0f;
    position = {0.f, 0.f, 0.f};
    setPosition(position);
    runSpeed = settings.playerRunSpeed;
    currentWeapon = WeaponType::FIST;
    lastAttackClock.restart();
    lastDamageTakenClock.restart();
    healthRegenDelayClock.restart();
    timeSinceLastCombatEvent.restart();
    dodgeTimer.restart();
    dodgeCooldownClock.restart();
}

void Player::toggleCrouch() {
    if (isAlive) {
        isCrouching = !isCrouching;
        if (isCrouching) isRunning = false;
    }
}

bool Player::dodge() {
    if (!isAlive || !inCombatStance || isDodging) return false;
    if (dodgeCooldownClock.getElapsedTime().asSeconds() < DODGE_COOLDOWN) return false;
    isDodging = true;
    dodgeTimer.restart();
    dodgeCooldownClock.restart();
    return true;
}

bool Player::isRegenOnCooldown(const GameSettings& settings) const {
    return healthRegenDelayClock.getElapsedTime().asSeconds() < settings.healthRegenDelay;
}
