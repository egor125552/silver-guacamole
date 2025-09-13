#include "Enemy.h"
#include "Player.h" // Include Player header
#include "SoundEngine.h"
#include <iostream>
#include <cmath> // For std::sqrt and std::pow
#include "Utils.h"

Enemy::Enemy(float x, float y, Player* player)
    : m_position(x, y),
      m_health(100),
      m_isStunned(false),
      m_state(State::PATROLLING),
      m_player(player),
      m_velocity(0.f, 0.f),
      m_speed(75.f),
      m_detectionRadius(300.f),
      m_attackRadius(50.f),
      m_currentPatrolIndex(0),
      m_isAlerted(false)
{
    m_shape.setRadius(20.f);
    m_shape.setFillColor(sf::Color::Red);
    m_shape.setPosition(m_position);

    // Define a simple patrol route
    m_patrolPoints.push_back({200.f, 150.f});
    m_patrolPoints.push_back({600.f, 150.f});
    m_patrolPoints.push_back({600.f, 450.f});
    m_patrolPoints.push_back({200.f, 450.f});
}

void Enemy::update(float deltaTime)
{
    if (m_isStunned)
    {
        if (m_stunClock.getElapsedTime() >= m_stunDuration)
        {
            m_isStunned = false;
            std::cout << "Enemy is no longer stunned." << std::endl;
        }
        m_velocity = {0.f, 0.f}; // Stop moving when stunned
    }
    else
    {
        // Simple state machine for AI
        switch (m_state)
        {
            case State::PATROLLING:
            {
                // Detection is more effective if player is running
                float currentDetectionRadius = m_detectionRadius;
                if (m_player->isRunning())
                {
                    currentDetectionRadius *= 2.0f;
                }

                // Check for player detection
                if (getDistance(m_position, m_player->getPosition()) < currentDetectionRadius)
                {
                    alert(); // This will set state to CHASING
                }
                else
                {
                    // Move towards the current patrol point
                    if (!m_patrolPoints.empty())
                    {
                        sf::Vector2f target = m_patrolPoints[m_currentPatrolIndex];
                        sf::Vector2f direction = target - m_position;

                        if (getDistance(m_position, target) < 5.f) // Reached the point
                        {
                            m_currentPatrolIndex = (m_currentPatrolIndex + 1) % m_patrolPoints.size();
                            m_velocity = {0.f, 0.f};
                        }
                        else
                        {
                            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                            if (length != 0) direction /= length;
                            m_velocity = direction * m_speed;
                        }
                    }
                }
                break;
            }
            case State::CHASING:
            {
                // If low health, start fleeing
                if (m_health < 30)
                {
                    m_state = State::FLEEING;
                    std::cout << "Enemy is low on health and has started fleeing." << std::endl;
                    break;
                }

                float distance = getDistance(m_position, m_player->getPosition());

                // Transition to attack if close enough
                if (distance < m_attackRadius)
                {
                    m_state = State::ATTACKING;
                    m_velocity = {0.f, 0.f}; // Stop to attack
                    std::cout << "Enemy is now attacking." << std::endl;
                    break;
                }

                // Lose player if too far
                if (distance > m_detectionRadius * 1.5f)
                {
                    m_state = State::PATROLLING;
                    std::cout << "Enemy lost player, returning to patrol." << std::endl;
                    break;
                }

                // Normal chase
                sf::Vector2f direction = m_player->getPosition() - m_position;
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length != 0) direction /= length;
                m_velocity = direction * m_speed * 1.5f; // Chase faster
                break;
            }
            case State::ATTACKING:
            {
                // If player moves out of attack range, go back to chasing
                if (getDistance(m_position, m_player->getPosition()) > m_attackRadius * 1.2f)
                {
                    m_state = State::CHASING;
                    std::cout << "Player moved out of range, enemy is chasing again." << std::endl;
                    break;
                }

                // Attack on a cooldown
                if (m_attackCooldown.getElapsedTime() >= m_attackInterval)
                {
                    std::cout << "Enemy attacks Player!" << std::endl;
                    m_player->takeDamage(10); // Enemy deals 10 damage
                    m_attackCooldown.restart();
                }
                break;
            }
            case State::FLEEING:
            {
                // If health is restored, go back to chasing
                if (m_health >= 30)
                {
                    m_state = State::CHASING;
                    std::cout << "Enemy recovered health, now chasing again." << std::endl;
                    break;
                }

                // If player is far away, go back to patrolling
                if (getDistance(m_position, m_player->getPosition()) > m_detectionRadius * 2.0f)
                {
                    m_state = State::PATROLLING;
                    std::cout << "Enemy successfully fled and is returning to patrol." << std::endl;
                    break;
                }

                // Move away from the player
                sf::Vector2f direction = m_position - m_player->getPosition();
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length != 0) direction /= length;
                m_velocity = direction * m_speed;
                break;
            }
            case State::IDLE:
            default:
                m_velocity = {0.f, 0.f};
                break;
        }
    }

    m_position += m_velocity * deltaTime;
    m_shape.setPosition(m_position);
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

    alert(); // Any damage alerts the enemy

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

sf::Vector2f Enemy::getPosition() const
{
    return m_position;
}

bool Enemy::isAlerted() const
{
    return m_isAlerted;
}

void Enemy::alert()
{
    if (!m_isAlerted)
    {
        m_isAlerted = true;
        m_state = State::CHASING; // Immediately start chasing when alerted
        std::cout << "Enemy has been alerted!" << std::endl;
    }
}
