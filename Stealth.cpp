#include "Stealth.h"
#include "Enemy.h"
#include "Player.h"
#include "common.h"
#include "utils.h"
#include <cmath>
#include <algorithm>

namespace {
int wallsBetween(const sf::Vector3f& from, const sf::Vector3f& to, const std::vector<sf::FloatRect>& walls)
{
    const sf::Vector2f start(from.x, from.z);
    const sf::Vector2f end(to.x, to.z);
    sf::Vector2f direction = end - start;
    const float distance = std::hypot(direction.x, direction.y);
    if (distance <= 0.001f) return 0;
    direction /= distance;

    int count = 0;
    for (const auto& wall : walls) {
        const sf::Vector2f hit = rayIntersectsRect(start, direction, wall);
        if (hit.x == -1.f) continue;
        const float hitDistance = std::hypot(hit.x - start.x, hit.y - start.y);
        if (hitDistance < distance - 0.05f) ++count;
    }
    return count;
}
}

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
    (void)settings;
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
    const std::vector<sf::FloatRect>& walls,
    float noiseDistance)
{
    if (noiseDistance <= 0.f) return;

    for (auto& enemy : enemies) {
        if (!enemy->isAlive || enemy->state == AIState::COMBAT) continue;

        const float distanceToEnemy = std::hypot(
            player.position.x - enemy->position.x,
            player.position.z - enemy->position.z);

        float hearingMultiplier = 1.0f;
        if (enemy->type == NPCType::GUARD) hearingMultiplier = 1.15f;
        else if (enemy->type == NPCType::SHOOTER) hearingMultiplier = 1.05f;
        else if (enemy->type == NPCType::BOSS) hearingMultiplier = 1.25f;

        const int blockingWalls = std::min(3, wallsBetween(player.position, enemy->position, walls));
        const float wallAttenuation = std::pow(0.48f, static_cast<float>(blockingWalls));
        const float effectiveRadius = noiseDistance * hearingMultiplier * wallAttenuation;

        if (distanceToEnemy < effectiveRadius) {
            enemy->investigate(player.position);
            // A very loud nearby noise should make the NPC more suspicious, but hearing alone
            // still does not magically grant exact combat lock through a wall.
            const float proximity = 1.0f - std::clamp(distanceToEnemy / std::max(effectiveRadius, 0.01f), 0.0f, 1.0f);
            enemy->detectionLevel = std::clamp(enemy->detectionLevel + 12.0f * proximity, 0.0f, 85.0f);
        }
    }
}

} // namespace StealthSystem
