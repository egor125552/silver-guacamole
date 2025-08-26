#include <gtest/gtest.h>
#include "SoundEngine.h"
#include "Enemy.h"
#include "Player.h"
#include <fstream>
#include <string>
#include "test_utils.h"
#include <string>


TEST(EnemyMovementTest, PlaysFootstepSoundsWhenInCombat) {
    // 1. Setup
    const std::string logFilePath = "error_log.txt";
    std::ofstream log_file(logFilePath, std::ofstream::out | std::ofstream::trunc);
    log_file.close();

    SoundEngine engine;
    engine.resetGame(); // We need walls for pathfinding

    Player player(engine.getSettings());
    // Place the player far away
    player.setPosition({100.f, 0.f, 100.f});

    // Create a single enemy to test
    Enemy enemy(sf::Vector3f(0, 0, 0), NPCType::GUARD, engine.getSettings());
    enemy.state = AIState::COMBAT; // Force combat state

    // 2. Action
    // Simulate ~2 seconds of game time
    const float simulationTime = 2.0f;
    const float deltaTime = 0.1f; // 10 updates per second
    for (float totalTime = 0; totalTime < simulationTime; totalTime += deltaTime) {
        // We need to pass a dummy vector of enemies for the update function
        std::vector<std::unique_ptr<Enemy>> allEnemies;
        enemy.update(deltaTime, player, engine, engine.getSettings(), GameMode::CLASSIC_ACTION, {}, allEnemies);
    }

    // 3. Assert
    std::string log_contents = readFileContents(logFilePath);

    // We expect the footstep sound to be logged multiple times.
    // The run interval is 0.4s, so in 2s we expect at least 4 sounds.
    int footstep_count = countOccurrences(log_contents, "Playing sound footstep");

    EXPECT_GE(footstep_count, 4)
        << "Expected at least 4 footstep sounds in combat, but found " << footstep_count;
}
