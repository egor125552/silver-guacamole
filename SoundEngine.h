#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <chrono>
#include <functional> // For std::function

#include "common.h"
#include "utils.h"

// Forward declarations
class NPC;
class Player;

enum class GameState {
    SelectingMode,
    Playing,
    PlayerDying,
    GameOver
};

struct Weapon {
    int playerDamage;
    float cooldown;
    std::string soundName;
    bool isAutomatic;
    float volume;
};

class SoundEngine {
public:
    SoundEngine();
    ~SoundEngine();

    void run();
    
    void playSound(const std::string& name, sf::Vector3f position, float volume = 100.f, bool isListenerRelative = false, float pitch = -1.f);
    void onNpcSpottedPlayer(NPC* spottedBy, bool forceCombat = false);
    void onNpcDied(NPC* deadNpc);
    bool hasLineOfSightTo(const sf::Vector3f& target);
    
    const GameSettings& getSettings() const { return settings; }

private:
    // --- Core Components ---
    sf::RenderWindow window;
    GameState gameState = GameState::SelectingMode;
    GameMode gameMode = GameMode::CLASSIC_ACTION;
    GameSettings settings;
    sf::Clock deltaClock;
    sf::Clock gameClock;

    // --- Game Entities ---
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<NPC>> npcs;
    std::vector<sf::FloatRect> walls;
    static constexpr int INITIAL_NPC_COUNT = 5;

    // --- Weapons ---
    std::map<WeaponType, Weapon> weapons;

    // --- Sound System ---
    static constexpr size_t SOUND_POOL_SIZE = 64;
    std::map<std::string, sf::SoundBuffer> soundBuffers;
    std::vector<sf::Sound> soundPool;
    size_t currentSoundIndex = 0;
    sf::SoundBuffer dummyBuffer;

    // --- Specialized Sounds ---
    sf::Sound playerDeathSound;
    sf::Sound sonarSound;
    sf::Sound lowHealthSound;
    sf::Sound detectionTickSound;
    sf::Sound breathingSound; // For stamina
    sf::Sound staminaCriticalSound; // For stamina

    // --- Timers ---
    sf::Clock sonarClock;
    sf::Clock proximitySonarClock;

    // --- Initialization ---
    void setupConsole();
    void loadSettings();
    void loadSounds();
    void loadSoundWithGeneration(const std::string& name, const std::string& path, std::function<sf::SoundBuffer()> generator);

    // --- Main Game Loop ---
    void processEvents();
    bool processInput(float deltaTime);
    void update(float deltaTime);
    
    // --- Game State Management ---
    void resetGame();
    void selectGameMode();
    void generateLevel();

    // --- Game Mechanics ---
    void handleNpcActions(float deltaTime);
    void handlePlayerAttack();
    void activateSonar();
    void activateDirectionalSonar(int numpadKey);
    void updateProximitySonar();
    void updateLowHealthSound();
    void updateStaminaSounds(); // New for stamina audio feedback
};

// Global function for logging
void logError(const std::string& message);
