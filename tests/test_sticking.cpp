#include <gtest/gtest.h>
#include "SoundEngine.h"
#include "Enemy.h"
#include "Player.h"
#include "test_utils.h"

TEST(StickingTest, EnemyPushesAwayWhenTooClose) {
    // 1. Setup
    SoundEngine engine;
    Player player(engine.getSettings());
    Enemy enemy(sf::Vector3f(0, 0, 0), NPCType::GUARD, engine.getSettings());

    // Place player and enemy at the exact same spot
    sf::Vector3f same_position = {10.f, 0.f, 10.f};
    player.setPosition(same_position);
    enemy.position = same_position;

    // Force combat state
    enemy.state = AIState::COMBAT;

    // 2. Action
    // Run a single update. This should be enough to trigger the push-away logic.
    std::vector<std::unique_ptr<Enemy>> allEnemies;
    std::vector<sf::FloatRect> walls;
    enemy.updateCombat(0.1f, player, engine, engine.getSettings(), walls, allEnemies);

    // 3. Assert
    // The enemy should no longer be at the exact same position as the player.
    EXPECT_NE(enemy.position, player.position)
        << "Enemy is still at the exact same position as the player after one update.";
}
