#include <gtest/gtest.h>
#include "SoundEngine.h"
#include "Enemy.h"
#include "Player.h"
#include "test_utils.h"
#include <fstream>
#include <string>

TEST(NpcAudio, PlaysSoundOnDeath) {
    // 1. Setup
    const std::string logFilePath = "error_log.txt";
    // Clear the log file
    std::ofstream log_file(logFilePath, std::ofstream::out | std::ofstream::trunc);
    log_file.close();

    // Create a SoundEngine instance. The OpenAL config file from build.sh will ensure it runs headlessly.
    SoundEngine engine;

    // Create a dummy player and enemy
    // We need a valid player pointer for the takeDamage function
    Player player(engine.getSettings());
    Enemy enemy(sf::Vector3f(10, 0, 0), NPCType::GUARD, engine.getSettings());

    // 2. Action
    // Kill the enemy. The damage amount should be greater than its health.
    enemy.takeDamage(1000, engine, &player);

    // 3. Assert
    // Check if the log file contains the expected message.
    std::string log_contents = readFileContents(logFilePath);

    // SoundEngine::playSound now logs automatically. We just need to check for the correct sound name.
    EXPECT_NE(log_contents.find("DEBUG_LOG: Playing sound death"), std::string::npos)
        << "The death sound log message was not found in the log file.";
}
