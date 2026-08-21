#include "Stealth.h"
#include "Enemy.h"
#include "Player.h"
#include "common.h"
#include <cmath>
#include <algorithm>

namespace StealthSystem
{

    bool isInShadow(const sf::Vector3f& position, const std::vector<sf::FloatRect>& shadowZones)
    {
        for (const auto& zone : shadowZones) {
            if (zone.contains({position.x, position.z})) return true;
        }
        return false;
    }

    void updateDetection(
        Enemy& enemy,
        const Player& player,
        const std::vector<sf::FloatRect>& walls,
        const std::vector<sf::FloatRect>& shadowZones,
        const GameSettings& settings,
        float deltaTime)
    {
        if (enemy.state == AIState::COMBAT || !player.isAlive) return;

        const float distance = std::hypot(player.position.x - enemy.position.x, player.position.z - enemy.position.z);
        float sightRange = 36.0f;
        if (enemy.type == NPCType::GUARD) sightRange = 48.0f;
        else if (enemy.type == NPCType::SHOOTER) sightRange = 55.0f;
        else if (enemy.type == NPCType::BOSS) sightRange = 60.0f;

        const bool hasLOS = distance <= sightRange && enemy.hasLineOfSight(player.position, walls);

        if (hasLOS) {
            float detectionRate = 25.0f;
            if (distance < 5.0f) detectionRate *= 3.0f;
            else if (distance > sightRange * 0.65f) detectionRate *= 0.45f;
            else if (distance > 20.0f) detectionRate *= 0.7f;

            if (isInShadow(player.position, shadowZones)) detectionRate *= 0.4f;
            if (player.isRunning) detectionRate *= 1.5f;
            else if (player.isCrouching) detectionRate *= 0.6f;

            enemy.detectionLevel += detectionRate * deltaTime;
        } else {
            enemy.detectionLevel -= 14.0f * deltaTime;
        }

        enemy.detectionLevel = std::clamp(enemy.detectionLevel, 0.0f, 100.0f);
    }

    void processPlayerNoise(
        const Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies,
        float noiseDistance)
    {
        if (noiseDistance <= 0) return;

        for (auto& enemy : enemies) {
            if (!enemy->isAlive || enemy->state == AIState::COMBAT) continue;

            const float distanceToEnemy = std::hypot(player.position.x - enemy->position.x, player.position.z - enemy->position.z);
            if (distanceToEnemy < noiseDistance) enemy->investigate(player.position);
        }
    }

} // namespace StealthSystem
