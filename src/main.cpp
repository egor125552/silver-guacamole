#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <memory>
#include <algorithm>
#include "Player.h"
#include "Enemy.h"
#include "SoundEngine.h"

int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Stealth Game");
    window.setFramerateLimit(60);

    Player player(400.f, 300.f);
    std::vector<std::unique_ptr<Enemy>> enemies;
    enemies.push_back(std::make_unique<Enemy>(200.f, 150.f));

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::Space)
                    player.attack(enemies);

                if (keyPressed->code >= sf::Keyboard::Key::Num1 && keyPressed->code <= sf::Keyboard::Key::Num5)
                {
                    int weaponIndex = static_cast<int>(keyPressed->code) - static_cast<int>(sf::Keyboard::Key::Num1);
                    player.switchToWeapon(weaponIndex);
                }
            }
        }

        player.update();
        for (auto& enemy : enemies)
        {
            enemy->update();
        }
        SoundEngine::getInstance()->update();

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [](const std::unique_ptr<Enemy>& e) {
                if (e->isDead()) {
                    SoundEngine::getInstance()->play("enemy_death.ogg");
                    return true;
                }
                return false;
            }), enemies.end());

        window.clear(sf::Color::Black);

        player.draw(window);
        for (auto& enemy : enemies)
        {
            enemy->draw(window);
        }

        window.display();
    }

    return 0;
}
