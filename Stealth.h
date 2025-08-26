#pragma once

#include <vector>
#include <memory>
#include <SFML/System/Vector3.hpp>
#include <SFML/Graphics/Rect.hpp>

// Forward declarations
class Enemy;
class Player;
struct GameSettings;

namespace StealthSystem
{
    /**
     * @brief Обновляет уровень обнаружения врага на основе текущего состояния игрока.
     * Эта функция будет вызываться для каждого врага в каждом кадре.
     * @param enemy Враг, для которого обновляется обнаружение.
     * @param player Игрок.
     * @param walls Геометрия уровня для проверки линии видимости.
     * @param shadowZones Зоны, в которых игрок менее заметен.
     * @param settings Глобальные настройки игры.
     * @param deltaTime Время, прошедшее с последнего кадра.
     */
    void updateDetection(
        Enemy& enemy,
        const Player& player,
        const std::vector<sf::FloatRect>& walls,
        const std::vector<sf::FloatRect>& shadowZones,
        const GameSettings& settings,
        float deltaTime
    );

    /**
     * @brief Обрабатывает шум, создаваемый игроком.
     * Оповещает ближайших врагов, которые могут услышать действие.
     * @param player Игрок, создавший шум.
     * @param enemies Список всех врагов на уровне.
     * @param noiseDistance Радиус шума.
     */
    void processPlayerNoise(
        const Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies,
        float noiseDistance
    );

    /**
     * @brief Проверяет, находится ли данная позиция в тени.
     * @param position Позиция для проверки.
     * @param shadowZones Вектор прямоугольников, представляющих тени.
     * @return true, если позиция находится в одной из теневых зон.
     */
    bool isInShadow(
        const sf::Vector3f& position,
        const std::vector<sf::FloatRect>& shadowZones
    );

} // namespace StealthSystem
