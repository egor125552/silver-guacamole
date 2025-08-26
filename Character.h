#pragma once

#include <SFML/System/Vector3.hpp>
#include <SFML/System/Clock.hpp>

// Forward declarations to avoid circular dependencies
class SoundEngine;
class Character;

#include "common.h" // For WeaponType, GameSettings etc.

class Character {
public:
    int health = 100;
    int maxHealth = 100;
    bool isAlive = true;
    sf::Vector3f position;

    bool isStunned = false;

    // Virtual destructor is important for base classes
    virtual ~Character() = default;

    // Made this virtual so Player and Enemy can have custom logic
    virtual bool takeDamage(int damage, SoundEngine& engine, Character* attacker = nullptr, bool guaranteedStun = false);

    // Clock to prevent taking damage multiple times from a single event
    sf::Clock lastAttackClock;

    // Stun logic
    virtual void stunFor(float duration);

protected:
    sf::Clock lastDamageTakenClock;
    sf::Clock stunClock;
    float currentStunDuration = 0.f;
};
