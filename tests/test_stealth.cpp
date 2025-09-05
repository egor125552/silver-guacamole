#include <gtest/gtest.h>
#include "SoundEngine.h"
#include "Enemy.h"
#include "Player.h"
#include "Stealth.h"
#include "test_utils.h"
#include <vector>
#include <memory>

TEST(StealthTest, BlocksDetectionThroughWall) {
    // 1. Setup
    // Create game settings and a player
    GameSettings settings;
    Player player(settings);
    player.setPosition({0.f, 0.f, 0.f});

    // Create an enemy on the other side of a conceptual wall
    Enemy enemy({10.f, 0.f, 0.f}, NPCType::GUARD, settings);

    // Create a wall that stands between the player and the enemy
    std::vector<sf::FloatRect> walls;
    walls.push_back(sf::FloatRect({5.f, -10.f}, {1.f, 20.f})); // Wall at x=5

    // Empty shadow zones for this test
    std::vector<sf::FloatRect> shadowZones;

    // 2. Action
    // Simulate ~2 seconds of game time to check for detection changes
    const float simulationTime = 2.0f;
    const float deltaTime = 0.1f;
    for (float totalTime = 0; totalTime < simulationTime; totalTime += deltaTime) {
        StealthSystem::updateDetection(enemy, player, walls, shadowZones, settings, deltaTime);
    }

    // 3. Assert
    // The enemy's detection level should remain at 0 because the wall blocks the line of sight.
    EXPECT_EQ(enemy.detectionLevel, 0.f)
        << "Enemy detection level should be 0 when a wall is blocking the view.";

    // The enemy should not have become alert or entered combat.
    EXPECT_NE(enemy.state, AIState::ALERT)
        << "Enemy should not be in ALERT state when a wall is blocking the view.";
    EXPECT_NE(enemy.state, AIState::COMBAT)
        << "Enemy should not be in COMBAT state when a wall is blocking the view.";
}
