#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

class Player; // Forward declaration

class Enemy
{
public:
    enum class State { IDLE, PATROLLING, CHASING, ATTACKING, FLEEING };

    Enemy(float x, float y, Player* player);

    void update(float deltaTime);
    void draw(sf::RenderWindow& window);

    sf::Vector2f getPosition() const;
    bool isAlerted() const;
    void alert();

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

    // AI-related members
    State m_state;
    Player* m_player;
    sf::Vector2f m_velocity;
    float m_speed;
    float m_detectionRadius;
    float m_attackRadius;

    std::vector<sf::Vector2f> m_patrolPoints;
    int m_currentPatrolIndex;

    sf::Clock m_attackCooldown;
    const sf::Time m_attackInterval = sf::seconds(1.5f);

    bool m_isAlerted;
};
