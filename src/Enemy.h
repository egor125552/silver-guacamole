#pragma once

#include <SFML/Graphics.hpp>

class Enemy
{
public:
    Enemy(float x, float y);

    void update();
    void draw(sf::RenderWindow& window);

    // Новые методы для боя
    void takeDamage(int amount);
    void stun();
    bool isStunned() const;
    bool isDead() const;

private:
    sf::Vector2f m_position;
    int m_health;
    sf::CircleShape m_shape;

    // Переменные для оглушения
    bool m_isStunned;
    sf::Clock m_stunClock;
    const sf::Time m_stunDuration = sf::seconds(5.f);
};
