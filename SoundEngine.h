#pragma once

#include <SFML/Graphics.hpp>
// #include <SFML/Audio.hpp> // Removed for pure OpenAL implementation
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <chrono>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>
#include <AL/efx-presets.h>

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
    ~SoundEngine();

    void run();
    
    // Публичные методы для взаимодействия с движком
    void playSound(const std::string& name, sf::Vector3f position, float volume = 100.f, bool isListenerRelative = false, float pitch = -1.f);
    void onEnemySpottedPlayer(Enemy* spottedBy, bool forceCombat = false);
    void onEnemyDied(Enemy* deadEnemy);
    bool hasLineOfSightTo(const sf::Vector3f& target);
    
    const GameSettings& getSettings() const { return settings; }
    void setReverbPreset(const EFXEAXREVERBPROPERTIES& preset);

    // Public methods for testing
    void _test_setPlayerPosition(const sf::Vector3f& pos);
    void _test_setEnemies(std::vector<std::unique_ptr<Enemy>>&& newEnemies);
    const Player* getPlayer() const { return player.get(); }
    const std::vector<std::unique_ptr<Enemy>>& getEnemies() const { return enemies; }
    void update(float deltaTime);
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
    ALCdevice* openalDevice = nullptr;
    ALCcontext* openalContext = nullptr;
    ALuint reverbEffect = 0;
    ALuint effectSlot = 0;

    static constexpr size_t SOUND_POOL_SIZE = 64;
    std::map<std::string, ALuint> openalBuffers;
    std::vector<ALuint> soundSources;
    size_t currentSourceIndex = 0;

    // Специализированные, постоянно играющие звуки
    ALuint lowHealthSoundSource = 0;
    ALuint shadowSoundSource = 0;
    
    // Таймеры для механик
    sf::Clock sonarClock;
    sf::Clock proximitySonarClock;

    // --- Состояние систем ---
    bool audioInitialized = false;

    // --- Инициализация ---
    void InitOpenAL();
    void ShutdownOpenAL();
    void setupConsole();
    void loadSettings();
    void loadSounds();
    void generateSounds();

    // --- Основной игровой цикл ---
    void processEvents();
    bool processInput(float deltaTime);
    // void update(float deltaTime); // Moved to public
    void render();
    
    // --- Управление состоянием игры ---
public:
    void resetGame();
private:
    void generateLevel();

    // --- Игровые механики ---
public:
    void handlePlayerAttack(); // Made public for testing
private:
    void handleEnemyActions(float deltaTime);
    void handlePlayerTakedown();
    void activateSonar();
    void activateDirectionalSonar(int numpadKey);
    void updateProximitySonar();
    void updateLowHealthSound();
    void updateShadowSound();
};

// Глобальная функция для логирования, т.к. она используется и в main
void logError(const std::string& message);