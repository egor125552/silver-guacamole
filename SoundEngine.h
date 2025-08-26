#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <chrono>

#include "common.h"
#include "utils.h"
#include "Stealth.h"

// Предварительные объявления, чтобы избежать циклических зависимостей
class Enemy;
class Player;

// Перечисление для состояний игры
enum class GameState {
    SelectingMode,
    Playing,
    PlayerDying,
    GameOver
};

// Структура для оружия вынесена сюда для чистоты
struct Weapon {
    int playerDamage;
    float cooldown;
    std::string soundName;
    bool isAutomatic;
    float volume;
};

// Основной класс игрового движка
class SoundEngine {
public:
    SoundEngine();
    ~SoundEngine(); // Объявляем деструктор для исправления ошибки компиляции

    void run();
    
    // Публичные методы для взаимодействия с движком
    void playSound(const std::string& name, sf::Vector3f position, float volume = 100.f, bool isListenerRelative = false, float pitch = -1.f);
    void onEnemySpottedPlayer(Enemy* spottedBy, bool forceCombat = false);
    void onEnemyDied(Enemy* deadEnemy);
    bool hasLineOfSightTo(const sf::Vector3f& target);
    
    const GameSettings& getSettings() const { return settings; }

private:
    // --- Основные компоненты ---
    sf::RenderWindow window;
    GameState gameState = GameState::Playing; // Start directly in the game
    GameMode gameMode = GameMode::CLASSIC_ACTION; // Use a single, unified mode
    GameSettings settings;
    sf::Clock deltaClock;
    sf::Clock gameClock;

    // --- Игровые сущности ---
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<sf::FloatRect> walls;
    std::vector<sf::FloatRect> shadowZones;
    static constexpr int INITIAL_NPC_COUNT = 20;

    // --- Оружие ---
    std::map<WeaponType, Weapon> weapons;

    // --- Звуковая система ---
    static constexpr size_t SOUND_POOL_SIZE = 64;
    std::map<std::string, sf::SoundBuffer> soundBuffers;
    std::vector<sf::Sound> soundPool;
    size_t currentSoundIndex = 0;
    sf::SoundBuffer dummyBuffer; // Для инициализации

    // Специализированные звуки
    sf::Sound playerDeathSound;
    sf::Sound sonarSound;
    sf::Sound lowHealthSound;
    sf::Sound detectionTickSound;
    sf::Sound shadowSound;
    
    // Таймеры для механик
    sf::Clock sonarClock;
    sf::Clock proximitySonarClock;

    // --- Состояние систем ---
    bool audioInitialized = false;

    // --- Инициализация ---
    void setupConsole();
    void loadSettings();
    void loadSounds();
    void generateSounds();

    // --- Основной игровой цикл ---
    void processEvents();
    bool processInput(float deltaTime);
    void update(float deltaTime);
    void render();
    
    // --- Управление состоянием игры ---
    void resetGame();
    void generateLevel();

    // --- Игровые механики ---
    void handleEnemyActions(float deltaTime);
    void handlePlayerAttack();
    void handlePlayerTakedown();
    void activateSonar();
    void activateDirectionalSonar(int numpadKey);
    void updateProximitySonar();
    void updateLowHealthSound();
    void updateShadowSound();
};

// Глобальная функция для логирования, т.к. она используется и в main
void logError(const std::string& message);