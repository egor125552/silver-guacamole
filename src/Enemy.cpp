#include "Enemy.h"
#include "SoundEngine.h"
#include <iostream>

Enemy::Enemy(float x, float y)
    : m_position(x, y),
      m_health(100),
      m_isStunned(false)
{
    m_shape.setRadius(20.f);
    m_shape.setFillColor(sf::Color::Red);
    m_shape.setPosition(m_position);
}

void Enemy::update()
{
    if (m_isStunned && m_stunClock.getElapsedTime() >= m_stunDuration)
    {
        m_isStunned = false;
        std::cout << "Enemy is no longer stunned." << std::endl;
    }
}

void Enemy::draw(sf::RenderWindow& window)
{
    m_shape.setFillColor(m_isStunned ? sf::Color::Blue : sf::Color::Red);
    window.draw(m_shape);
}

void Enemy::takeDamage(int amount)
{
    if (isDead()) return;

    m_health -= amount;
    SoundEngine::getInstance()->play("enemy_hit.ogg");
    std::cout << "Enemy takes " << amount << " damage. Health: " << m_health << std::endl;
    if (isDead())
    {
        // Звук смерти будет проигрываться в main.cpp при удалении
        std::cout << "Enemy has died." << std::endl;
    }
}

void Enemy::stun()
{
    if (isStunned() || isDead()) return;

    m_isStunned = true;
    m_stunClock.restart();
    SoundEngine::getInstance()->play("stun.ogg");
    std::cout << "Enemy is stunned for 5 seconds." << std::endl;
}

bool Enemy::isStunned() const
{
    return m_isStunned;
}

bool Enemy::isDead() const
{
    return m_health <= 0;
}
