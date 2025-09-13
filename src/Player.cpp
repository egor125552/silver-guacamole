#include "Player.h"
#include "Enemy.h"
#include "SoundEngine.h" // Уже подключен для атаки
#include <iostream>
#include <limits> // For std::numeric_limits
#include <cmath>  // For std::sqrt & std::pow
#include "Utils.h"

#include "Fists.h"
#include "Pistol.h"
#include "Shocker.h"
#include "Knife.h"
#include "Machete.h"

Player::Player(float x, float y)
    : m_position(x, y),
      m_velocity(0.f, 0.f),
      m_walkSpeed(100.f),
      m_runSpeed(200.f),
      m_isRunning(false),
      m_health(100),
      m_currentWeaponIndex(0),
      m_isStunned(false)
{
    m_shape.setRadius(20.f);
    m_shape.setFillColor(sf::Color::Green);
    m_shape.setPosition(m_position);

    m_weapons.push_back(std::make_unique<Fists>());
    m_weapons.push_back(std::make_unique<Pistol>());
    m_weapons.push_back(std::make_unique<Shocker>());
    m_weapons.push_back(std::make_unique<Knife>());
    m_weapons.push_back(std::make_unique<Machete>());
}

void Player::handleInput()
{
    if (isStunned())
    {
        m_isRunning = false;
        return;
    }

    m_velocity = {0.f, 0.f};
    m_isRunning = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);

    float currentSpeed = m_isRunning ? m_runSpeed : m_walkSpeed;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
    {
        m_velocity.y -= currentSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
    {
        m_velocity.y += currentSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
    {
        m_velocity.x -= currentSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
    {
        m_velocity.x += currentSpeed;
    }
}

void Player::update(float deltaTime)
{
    if (m_isStunned && m_stunClock.getElapsedTime() >= m_stunDuration)
    {
        m_isStunned = false;
        std::cout << "Player is no longer stunned." << std::endl;
    }

    if (!isStunned())
    {
        m_position += m_velocity * deltaTime;
        m_shape.setPosition(m_position);
    }
}

void Player::draw(sf::RenderWindow& window)
{
    m_shape.setFillColor(m_isStunned ? sf::Color::Yellow : sf::Color::Green);
    window.draw(m_shape);
}

void Player::attack(std::vector<std::unique_ptr<Enemy>>& enemies)
{
    if (isStunned() || isDead() || enemies.empty()) return;

    // Find the closest enemy
    Enemy* closestEnemy = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (const auto& enemy : enemies)
    {
        float distance = getDistance(m_position, enemy->getPosition());
        if (distance < minDistance)
        {
            minDistance = distance;
            closestEnemy = enemy.get();
        }
    }

    if (closestEnemy && minDistance < 100.f) // Use a fixed attack range for now
    {
        Weapon* currentWeapon = m_weapons[m_currentWeaponIndex].get();
        currentWeapon->attack(); // Play attack sound etc.

        // Stealth kill condition: silent weapon AND (target is not alerted OR target is stunned)
        if (currentWeapon->isSilent() && (!closestEnemy->isAlerted() || closestEnemy->isStunned()))
        {
            std::cout << "Player performs a stealth takedown!" << std::endl;
            closestEnemy->takeDamage(999); // Instant kill
        }
        else // Normal attack
        {
            int damage = currentWeapon->getDamage();
            closestEnemy->takeDamage(damage);

            if (currentWeapon->isStunWeapon())
            {
                closestEnemy->stun();
            }
        }
    }
}

void Player::switchToWeapon(size_t index)
{
    if (isStunned()) return;

    if (index < m_weapons.size() && index != m_currentWeaponIndex)
    {
        m_currentWeaponIndex = index;
        std::cout << "Player switched to " << m_weapons[m_currentWeaponIndex]->getName() << std::endl;
        SoundEngine::getInstance()->play("switch_weapon.ogg");
    }
}

void Player::takeDamage(int amount)
{
    if (isDead()) return;

    m_health -= amount;
    SoundEngine::getInstance()->play("player_hit.ogg");
    std::cout << "Player takes " << amount << " damage. Health: " << m_health << std::endl;
    if (isDead())
    {
        std::cout << "Player has died." << std::endl;
        SoundEngine::getInstance()->play("player_death.ogg");
    }
}

void Player::stun()
{
    if (isStunned() || isDead()) return;

    m_isStunned = true;
    m_stunClock.restart();
    SoundEngine::getInstance()->play("stun.ogg");
    std::cout << "Player is stunned for 5 seconds." << std::endl;
}

bool Player::isStunned() const
{
    return m_isStunned;
}

bool Player::isDead() const
{
    return m_health <= 0;
}

sf::Vector2f Player::getPosition() const
{
    return m_position;
}

bool Player::isRunning() const
{
    return m_isRunning;
}
