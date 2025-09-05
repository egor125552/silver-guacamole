#include <gtest/gtest.h>
#include "SoundEngine.h"
#include "Enemy.h"
#include "Player.h"
#include "test_utils.h"
#include <vector>
#include <memory>

TEST(WeaponTest, PistolDealsCorrectDamage) {
    // 1. Setup
    SoundEngine engine;
    const auto& settings = engine.getSettings();

    // Get the player and place them at the origin
    Player* player = const_cast<Player*>(engine.getPlayer());
    player->setPosition({0.f, 0.f, 0.f});
    player->switchWeapon(WeaponType::PISTOL);

    // Create a single enemy for our test scenario
    auto testEnemy = std::make_unique<Enemy>(sf::Vector3f(1.f, 0.f, 0.f), NPCType::GUARD, settings);
    float enemyInitialHealth = testEnemy->health;

    // Create a vector and move our test enemy into it
    std::vector<std::unique_ptr<Enemy>> newEnemies;
    newEnemies.push_back(std::move(testEnemy));

    // Use the test helper to replace the engine's enemies with our controlled set
    engine._test_setEnemies(std::move(newEnemies));

    // 2. Action
    // handlePlayerAttack should find and damage the only enemy we have placed.
    engine.handlePlayerAttack();

    // 3. Assert
    // The health of our enemy should be reduced by the pistol's damage amount.
    const auto& finalEnemies = engine.getEnemies();
    ASSERT_EQ(finalEnemies.size(), 1) << "There should only be one enemy in the engine.";

    float expectedHealth = enemyInitialHealth - settings.pistolDamage;
    EXPECT_EQ(finalEnemies[0]->health, expectedHealth)
        << "Enemy health should be reduced by the pistol's damage amount.";
}
