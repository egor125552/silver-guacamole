#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include "Weapon.h"

// Forward-declare Enemy to avoid circular dependency
class Enemy;

class Player
{
public:
    Player(float x, float y);

    void handleInput();
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);

    // Сигнатура изменена на прием вектора unique_ptr
    void attack(std::vector<std::unique_ptr<Enemy>>& enemies);
    void switchToWeapon(size_t index);

    void takeDamage(int amount);
    void stun();
    bool isStunned() const;
    bool isDead() const;

    sf::Vector2f getPosition() const;
    bool isRunning() const;

private:
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    float m_walkSpeed;
    float m_runSpeed;
    bool m_isRunning;
    int m_health;
    sf::CircleShape m_shape;

    std::vector<std::unique_ptr<Weapon>> m_weapons;
    size_t m_currentWeaponIndex;

    bool m_isStunned;
    sf::Clock m_stunClock;
    const sf::Time m_stunDuration = sf::seconds(5.f);
};
