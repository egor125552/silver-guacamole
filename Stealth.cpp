#include "Stealth.h"
#include "Enemy.h"
#include "Player.h"
#include "common.h"
#include <cmath>

namespace StealthSystem
{

    bool isInShadow(const sf::Vector3f& position, const std::vector<sf::FloatRect>& shadowZones)
    {
        for (const auto& zone : shadowZones) {
            if (zone.contains({position.x, position.z})) {
                return true;
            }
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
        // Логика обнаружения не применяется, если враг уже в бою или игрок мертв
        if (enemy.state == AIState::COMBAT || !player.isAlive) {
            return;
        }

        float distance = std::hypot(player.position.x - enemy.position.x, player.position.z - enemy.position.z);

        // TODO: Добавить более сложную логику, учитывающую угол обзора врага
        bool hasLOS = enemy.hasLineOfSight(player.position, walls);

        if (hasLOS) {
            // Базовая скорость обнаружения
            float detectionRate = 25.0f; // очков в секунду на среднем расстоянии

            // Модификатор расстояния: чем ближе, тем быстрее обнаружение
            if (distance < 5.0f) detectionRate *= 3.0f; // Очень близко
            else if (distance > 20.0f) detectionRate *= 0.5f; // Далеко

            // Модификатор освещения
            if (isInShadow(player.position, shadowZones)) {
                detectionRate *= 0.4f; // В тени обнаружение на 60% медленнее
            }

            // Модификатор движения игрока
            if (player.isRunning) {
                detectionRate *= 1.5f;
            } else if (player.isCrouching) {
                detectionRate *= 0.6f;
            }

            enemy.detectionLevel += detectionRate * deltaTime;

        } else {
            // Если игрока не видно, уровень обнаружения медленно падает
            enemy.detectionLevel -= 10.0f * deltaTime;
        }

        // Удерживаем уровень обнаружения в пределах 0-100
        if (enemy.detectionLevel < 0) enemy.detectionLevel = 0;
        if (enemy.detectionLevel > 100) enemy.detectionLevel = 100;
    }

    void processPlayerNoise(
        const Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies,
        float noiseDistance)
    {
        if (noiseDistance <= 0) return;

        for (auto& enemy : enemies) {
            // Шум не влияет на тех, кто уже в бою или мертв
            if (!enemy->isAlive || enemy->state == AIState::COMBAT) {
                continue;
            }

            float distanceToEnemy = std::hypot(player.position.x - enemy->position.x, player.position.z - enemy->position.z);

            if (distanceToEnemy < noiseDistance) {
                // Враг услышал шум, переводим его в состояние тревоги
                enemy->investigate(player.position);
            }
        }
    }

} // namespace StealthSystem
