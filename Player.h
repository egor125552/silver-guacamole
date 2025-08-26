#pragma once

#include "Character.h"
#include <vector>
#include <memory>

// Forward declaration
class Enemy;

class Player : public Character {
public:
    static constexpr float WALK_SPEED = 5.0f;
    static constexpr float CROUCH_SPEED = 2.0f;
    static constexpr float WALK_STEP_INTERVAL = 0.55f;
    static constexpr float RUN_STEP_INTERVAL = 0.35f;
    static constexpr float CROUCH_STEP_INTERVAL = 0.8f;

    float runSpeed = 18.0f;
    bool isRunning = false;
    bool godMode = false;
    bool isCrouching = false;
    WeaponType currentWeapon = WeaponType::FIST;
    bool inCombatStance = false;
    bool isDodging = false;

private:
    float healthRegenBuffer = 0.0f;
    sf::Clock healthRegenDelayClock;
    sf::Clock timeSinceLastCombatEvent;
    sf::Clock dodgeTimer;

public:
    explicit Player(const GameSettings& settings);
    void update(float deltaTime, const GameSettings& settings, const std::vector<std::unique_ptr<Enemy>>& enemies);
    void setPosition(const sf::Vector3f& newPos);
    void switchWeapon(WeaponType newWeapon);
    void reset(const GameSettings& settings);
    bool takeDamage(int damage, SoundEngine& engine, Character* attacker = nullptr, bool guaranteedStun = false) override;
    void toggleCrouch();
    void dodge();
    bool isRegenOnCooldown(const GameSettings& settings) const;
};
