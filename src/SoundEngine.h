#pragma once

#include <SFML/Audio.hpp>
#include <string>
#include <map>
#include <list> // Для std::list

class SoundEngine
{
public:
    static SoundEngine* getInstance();

    void play(const std::string& filename);
    void update(); // Для очистки отыгравших звуков

    // Запрещаем копирование и присваивание
    SoundEngine(const SoundEngine&) = delete;
    void operator=(const SoundEngine&) = delete;

private:
    SoundEngine();
    ~SoundEngine() = default;

    std::map<std::string, sf::SoundBuffer> m_soundBuffers;
    std::list<sf::Sound> m_sounds; // Список проигрываемых звуков
};
