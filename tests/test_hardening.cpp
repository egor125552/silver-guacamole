#include <gtest/gtest.h>
#include <SFML/System.hpp>
#include "Enemy.h"
#include "Player.h"
#include "Stealth.h"
#include "common.h"

TEST(Hardening, EnemyHealthUsesConfiguredTypeValues) {
    GameSettings settings;
    settings.regularHealth = 123;
    settings.shooterHealth = 177;
    settings.bossHealth = 555;

    Enemy regular({10.f, 0.f, 0.f}, NPCType::REGULAR, settings);
    Enemy shooter({20.f, 0.f, 0.f}, NPCType::SHOOTER, settings);
    Enemy guard({30.f, 0.f, 0.f}, NPCType::GUARD, settings);
    Enemy boss({40.f, 0.f, 0.f}, NPCType::BOSS, settings);

    EXPECT_EQ(regular.maxHealth, 123);
    EXPECT_EQ(regular.health, 123);
    EXPECT_EQ(shooter.maxHealth, 177);
    EXPECT_EQ(shooter.health, 177);
    EXPECT_EQ(guard.maxHealth, 177);
    EXPECT_EQ(guard.health, 177);
    EXPECT_EQ(boss.maxHealth, 555);
    EXPECT_EQ(boss.health, 555);
}

TEST(Hardening, GuardLoadoutIsOneOfConfiguredRoles) {
    GameSettings settings;
    for (int i = 0; i < 100; ++i) {
        Enemy guard({10.f, 0.f, 0.f}, NPCType::GUARD, settings);
        const bool validWeapon =
            guard.weapon == WeaponType::PISTOL ||
            guard.weapon == WeaponType::AUTOMATIC ||
            guard.weapon == WeaponType::TASER ||
            guard.weapon == WeaponType::BATON;
        EXPECT_TRUE(validWeapon);
        if (guard.weapon == WeaponType::BATON) EXPECT_EQ(guard.behavior, AIBehavior::AGGRESSOR);
        else EXPECT_EQ(guard.behavior, AIBehavior::SUPPORT);
    }
}

TEST(Hardening, FarEnemyCannotBuildDetectionAcrossMap) {
    GameSettings settings;
    Player player(settings);
    player.setPosition({-90.f, 0.f, 0.f});
    Enemy guard({90.f, 0.f, 0.f}, NPCType::GUARD, settings);
    std::vector<sf::FloatRect> walls;
    std::vector<sf::FloatRect> shadows;

    for (int i = 0; i < 200; ++i) {
        StealthSystem::updateDetection(guard, player, walls, shadows, settings, 0.1f);
    }

    EXPECT_FLOAT_EQ(guard.detectionLevel, 0.f);
    EXPECT_NE(guard.state, AIState::COMBAT);
}

TEST(Hardening, ShadowAndCrouchReduceDetection) {
    GameSettings settings;
    Player litPlayer(settings);
    Player hiddenPlayer(settings);
    litPlayer.setPosition({10.f, 0.f, 0.f});
    hiddenPlayer.setPosition({10.f, 0.f, 0.f});
    hiddenPlayer.isCrouching = true;

    Enemy litEnemy({0.f, 0.f, 0.f}, NPCType::REGULAR, settings);
    Enemy hiddenEnemy({0.f, 0.f, 0.f}, NPCType::REGULAR, settings);
    std::vector<sf::FloatRect> walls;
    std::vector<sf::FloatRect> noShadows;
    std::vector<sf::FloatRect> shadows{sf::FloatRect({5.f, -5.f}, {10.f, 10.f})};

    for (int i = 0; i < 10; ++i) {
        StealthSystem::updateDetection(litEnemy, litPlayer, walls, noShadows, settings, 0.1f);
        StealthSystem::updateDetection(hiddenEnemy, hiddenPlayer, walls, shadows, settings, 0.1f);
    }

    EXPECT_GT(litEnemy.detectionLevel, hiddenEnemy.detectionLevel);
    EXPECT_GT(litEnemy.detectionLevel, 0.f);
    EXPECT_GE(hiddenEnemy.detectionLevel, 0.f);
}

TEST(Hardening, DodgeCannotBeSpammed) {
    GameSettings settings;
    Player player(settings);
    player.inCombatStance = true;

    EXPECT_FALSE(player.dodge());
    sf::sleep(sf::milliseconds(1300));
    EXPECT_TRUE(player.dodge());
    EXPECT_FALSE(player.dodge());
}
