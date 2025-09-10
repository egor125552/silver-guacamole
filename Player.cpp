#include "Player.h"
#include "SoundEngine.h"
#include "Enemy.h"
#include <algorithm> // For std::min
#include <cmath> // For std::hypot
#include <AL/al.h> // For alListener3f

Player::Player(const GameSettings& settings) {
    reset(settings);
}

void Player::update(float deltaTime, const GameSettings& settings, const std::vector<std::unique_ptr<Enemy>>& enemies) {
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
        for (const auto& enemy : enemies) {
            if (enemy->isAlive && enemy->state == AIState::COMBAT) {
                float distance = std::hypot(position.x - enemy->position.x, position.z - enemy->position.z);
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

    // Health regeneration logic
    if (health < maxHealth && lastAttackClock.getElapsedTime().asSeconds() > settings.healthRegenDelay) {
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
    if (isAlive) {
        currentWeapon = newWeapon;
    }
}

bool Player::takeDamage(int damage, SoundEngine& engine, Character* attacker, bool guaranteedStun) {
    if (godMode || isDodging || lastDamageTakenClock.getElapsedTime().asSeconds() < 0.2f) return false;

    if (guaranteedStun) {
        stunFor(5.f); // Stun player for 5 seconds
        // We can reuse the NPC stun sound for the player for now
        engine.playSound("Stun", {0,0,0}, 100.f, true);
    }

    lastAttackClock.restart(); // Resetting this clock starts the regeneration delay
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

void Player::toggleCrouch() {
    if (isAlive) {
        isCrouching = !isCrouching;
        if (isCrouching) {
            isRunning = false;
        }
    }
}

void Player::dodge() {
    if (isAlive && inCombatStance && !isDodging) {
        isDodging = true;
        dodgeTimer.restart();
        // Maybe play a sound here in the future
    }
}

bool Player::isRegenOnCooldown(const GameSettings& settings) const {
    return healthRegenDelayClock.getElapsedTime().asSeconds() < settings.healthRegenDelay;
}
