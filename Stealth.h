#pragma once

#include <vector>
#include <memory>
#include <SFML/System/Vector3.hpp>
#include <SFML/Graphics/Rect.hpp>

class Enemy;
class Player;
struct GameSettings;

namespace StealthSystem
{
    void updateDetection(
        Enemy& enemy,
        const Player& player,
        const std::vector<sf::FloatRect>& walls,
        const std::vector<sf::FloatRect>& shadowZones,
        const GameSettings& settings,
        float deltaTime
    );

    // Noise now uses the same level geometry as vision. Walls muffle sound instead of
    // letting every NPC hear the player through solid rooms at the full radius.
    void processPlayerNoise(
        const Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies,
        const std::vector<sf::FloatRect>& walls,
        float noiseDistance
    );

    bool isInShadow(
        const sf::Vector3f& position,
        const std::vector<sf::FloatRect>& shadowZones
    );

} // namespace StealthSystem
