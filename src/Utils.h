#pragma once

#include <SFML/System/Vector2.hpp>
#include <cmath>

inline float getDistance(const sf::Vector2f& pos1, const sf::Vector2f& pos2)
{
    return std::sqrt(std::pow(pos2.x - pos1.x, 2) + std::pow(pos2.y - pos1.y, 2));
}
