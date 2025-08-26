#include "Character.h"
#include "SoundEngine.h" // Included for takeDamage signature

// A forward declaration from SoundEngine.cpp for logging.
// This is not ideal, but it's how the project is currently structured.
// A better solution would be a dedicated logging utility.
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
    if (health <= 0) {
        health = 0;
        isAlive = false;
    }
    return true;
}
