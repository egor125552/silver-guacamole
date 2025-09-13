#include "SoundEngine.h"
#include <iostream>

SoundEngine* SoundEngine::getInstance()
{
    static SoundEngine instance;
    return &instance;
}

SoundEngine::SoundEngine()
{
    // Конструктор
}

void SoundEngine::play(const std::string& filename)
{
    auto it = m_soundBuffers.find(filename);
    if (it == m_soundBuffers.end())
    {
        sf::SoundBuffer buffer;
        if (buffer.loadFromFile("sounds/" + filename))
        {
            m_soundBuffers[filename] = buffer;
            it = m_soundBuffers.find(filename);
        }
        else
        {
            std::cerr << "Ошибка загрузки звукового файла: " << filename << std::endl;
            return;
        }
    }

    m_sounds.emplace_back(it->second);
    m_sounds.back().play();
}

void SoundEngine::update()
{
    // Удаляем звуки, которые закончили проигрываться
    // Используем sf::Sound::Status::Stopped
    m_sounds.remove_if([](const sf::Sound& s) {
        return s.getStatus() == sf::Sound::Status::Stopped;
    });
}
