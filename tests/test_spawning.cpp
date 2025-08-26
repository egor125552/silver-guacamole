#include <gtest/gtest.h>
#include "SoundEngine.h"
#include "Player.h"
#include "Enemy.h"
#include <cmath>

// Test fixture for spawning logic
class SpawningTest : public ::testing::Test {
protected:
    // Helper function to calculate distance on the XZ plane
    float distance(const sf::Vector3f& p1, const sf::Vector3f& p2) {
        return std::hypot(p1.x - p2.x, p1.z - p2.z);
    }
};

TEST_F(SpawningTest, EnemiesAreNotSpawnedTooCloseToPlayer) {
    // 1. Setup
    // SoundEngine constructor creates the object, but we need to explicitly generate a level for the test.
    SoundEngine engine;
    engine.resetGame(); // This will generate the level and spawn enemies
    const float MIN_SPAWN_DISTANCE = 15.0f; // Let's define a safe distance

    // Retrieve player and enemies using the new getter methods
    const Player* player = engine.getPlayer();
    const auto& enemies = engine.getEnemies();

    // Make sure we actually have a player and enemies to test
    ASSERT_NE(player, nullptr);
    ASSERT_FALSE(enemies.empty());

    // 2. Action & Assert
    // Iterate through all enemies and check their distance from the player
    for (const auto& enemy : enemies) {
        ASSERT_NE(enemy, nullptr);
        float dist = distance(player->position, enemy->position);

        // The core assertion of the test
        EXPECT_GE(dist, MIN_SPAWN_DISTANCE)
            << "Enemy spawned at (" << enemy->position.x << ", " << enemy->position.z
            << "), which is only " << dist << " units away from the player at ("
            << player->position.x << ", " << player->position.z << "). "
            << "Minimum distance should be " << MIN_SPAWN_DISTANCE;
    }
}
