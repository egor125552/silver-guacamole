#include <gtest/gtest.h>
#include "SoundEngine.h"
#include "Enemy.h"
#include "Player.h"
#include "common.h"
#include <vector>
#include <memory>

TEST(NPCBehaviorTest, MovesTowardsPlayerInCombat) {
    // 1. Setup
    // We need a SoundEngine instance because the Enemy::update method calls engine.playSound
    SoundEngine engine;
    auto settings = engine.getSettings();
    settings.npcRunSpeed = 5.0f; // Set a predictable speed

    // Create a player at the origin
    Player player(settings);
    player.setPosition({0.f, 0.f, 0.f});

    // Create an enemy and place it 10 units away on the x-axis
    Enemy enemy({10.f, 0.f, 0.f}, NPCType::GUARD, settings);
    enemy.state = AIState::COMBAT; // Force combat state

    // Empty vectors for walls and other enemies, as they are not needed for this test
    std::vector<sf::FloatRect> no_walls;
    std::vector<std::unique_ptr<Enemy>> no_other_enemies;

    // 2. Action
    // Simulate one second of movement
    float deltaTime = 0.1f;
    for (int i = 0; i < 10; ++i) { // 10 * 0.1s = 1 second
        enemy.update(deltaTime, player, engine, settings, GameMode::CLASSIC_ACTION, no_walls, no_other_enemies);
    }

    // 3. Assert
    // The enemy should have moved towards the player at the origin.
    // Its x position should be less than the starting position of 10.
    // With a run speed of 5.0, after 1 second it should be at x = 5.0.
    EXPECT_LT(enemy.position.x, 10.0f)
        << "Enemy should have moved towards the player (new X position should be less than initial X).";

    EXPECT_NEAR(enemy.position.x, 5.0f, 0.1f)
        << "Enemy should be at x=5 after running towards origin from x=10 for 1s at 5 units/sec.";
}
