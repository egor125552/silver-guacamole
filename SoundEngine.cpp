#include "SoundEngine.h"
#include "Player.h"
#include "Enemy.h"
#include "utils.h"
#include "ai_definitions.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include <optional>

#ifdef _WIN32
#include <windows.h>
#endif

// TODO: Future feature - Vehicle system
// We will need classes for cars, motorcycles, etc.
// And a physics system to handle their movement.

// TODO: Future feature - Mission system
// A system to define objectives, track progress, and give rewards.
// Can be scripted using another language like Lua in the future.

// TODO: Future feature - Music system
// A system to play background music that changes based on the situation
// (e.g., stealth, combat, exploring).

void logError(const std::string& message) {
    std::ofstream log_file("error_log.txt", std::ios_base::app);
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    char time_str[26];
#ifdef _WIN32
    struct tm tm_buf;
    localtime_s(&tm_buf, &time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);
#else
    struct tm tm_buf;
    localtime_r(&time, &tm_buf);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);
#endif
    log_file << "[" << time_str << "] " << message << std::endl;
}

SoundEngine::SoundEngine()
    : window(sf::VideoMode({800, 600}), "Stealth Action - Coordinated Assault", sf::Style::Titlebar | sf::Style::Close),
      playerDeathSound(dummyBuffer), sonarSound(dummyBuffer),
      lowHealthSound(dummyBuffer), detectionTickSound(dummyBuffer), shadowSound(dummyBuffer)
{
    logError("DEBUG_LOG: Constructor start.");
    setupConsole();
    logError("DEBUG_LOG: Console setup.");
    loadSettings();
    logError("DEBUG_LOG: Settings loaded.");

    try {
        audioInitialized = true;
        logError("DEBUG_LOG: Loading sounds...");
        loadSounds();
        logError("DEBUG_LOG: Sounds loaded.");
        logError("DEBUG_LOG: Generating sounds...");
        generateSounds();
        logError("DEBUG_LOG: Sounds generated.");

        playerDeathSound.setBuffer(soundBuffers.at("player_death"));
        playerDeathSound.setRelativeToListener(true);
        sonarSound.setBuffer(soundBuffers.at("sonar"));
        lowHealthSound.setBuffer(soundBuffers.at("LowHealth"));
        lowHealthSound.setRelativeToListener(true);
        lowHealthSound.setLooping(true);
        lowHealthSound.setVolume(80);
        detectionTickSound.setBuffer(soundBuffers.at("DetectionTick"));
        detectionTickSound.setRelativeToListener(true);
        shadowSound.setBuffer(soundBuffers.at("Shadow_Ambience"));
        shadowSound.setRelativeToListener(true);
        shadowSound.setLooping(true);
        shadowSound.setVolume(30);
        logError("DEBUG_LOG: Special sounds initialized.");

    } catch (const std::exception& e) {
        logError("AUDIO_ERROR: Failed to initialize sounds. Game will run without audio. Error: " + std::string(e.what()));
        audioInitialized = false;
        soundBuffers.clear();
    }

    logError("DEBUG_LOG: Creating player...");
    player = std::make_unique<Player>(settings);
    logError("DEBUG_LOG: Player created.");

    soundPool.reserve(SOUND_POOL_SIZE);
    for (size_t i = 0; i < SOUND_POOL_SIZE; ++i) {
        soundPool.emplace_back(dummyBuffer);
    }
    logError("DEBUG_LOG: Sound pool created.");

    // Weapon definitions
    weapons[WeaponType::FIST]      = {settings.fistDamage, 0.5f, "punch", false, settings.fistVolume};
    weapons[WeaponType::PISTOL]    = {settings.pistolDamage, 0.4f, "pistol", false, settings.pistolVolume};
    weapons[WeaponType::TASER]     = {settings.taserDamage, settings.taserCooldown, "Taser_Fire", false, settings.taserVolume};
    weapons[WeaponType::AUTOMATIC] = {settings.automaticDamage, 0.1f, "automatic", true, settings.automaticVolume};
    weapons[WeaponType::SNIPER]    = {settings.sniperDamage, 1.5f, "sniper", false, settings.sniperVolume};
    weapons[WeaponType::MACHETE]   = {settings.macheteDamage, 0.7f, "Machete_Swish", false, settings.macheteVolume};
    weapons[WeaponType::KNIFE]     = {settings.knifeDamage, 0.4f, "Knife_Swish", false, settings.knifeVolume};
    weapons[WeaponType::CROWBAR]   = {settings.crowbarDamage, 0.9f, "Blunt_Metal_Swish", false, settings.crowbarVolume};
    weapons[WeaponType::BAT]       = {settings.batDamage, 0.8f, "Bat_Swish", false, settings.batVolume};
    weapons[WeaponType::SHANK]     = {settings.shankDamage, 0.3f, "Knife_Swish", false, settings.shankVolume};
    weapons[WeaponType::BATON]     = {settings.batonDamage, 0.6f, "Blunt_Metal_Swish", false, settings.batonVolume};
    logError("DEBUG_LOG: Weapon definitions loaded.");

    logError("DEBUG_LOG: Constructor end.");
}

SoundEngine::~SoundEngine() = default;

void SoundEngine::loadSettings() {
    std::ifstream file("config.ini");
    if (!file.is_open()) {
        logError("Warning: config.ini not found. Using default values.");
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            key.erase(remove_if(key.begin(), key.end(), isspace), key.end());
            value.erase(remove_if(value.begin(), value.end(), isspace), value.end());
            if (key.empty() || key[0] == ';') continue;
            try {
                if (key == "Health") settings.playerHealth = std::stoi(value);
                else if (key == "PlayerRunSpeed") settings.playerRunSpeed = std::stof(value);
                else if (key == "HealthRegenRate") settings.healthRegenRate = std::stof(value);
                else if (key == "HealthRegenDelay") settings.healthRegenDelay = std::stof(value);
                else if (key == "LowHealthThreshold") settings.lowHealthThreshold = std::stoi(value);
                else if (key == "RegularHealth") settings.regularHealth = std::stoi(value);
                else if (key == "ShooterHealth") settings.shooterHealth = std::stoi(value);
                else if (key == "MeleeNpcCanAttack") settings.meleeNpcCanAttack = (value == "yes");
                else if (key == "NpcWalkSpeed") settings.npcWalkSpeed = std::stof(value);
                else if (key == "NpcRunSpeed") settings.npcRunSpeed = std::stof(value);
                else if (key == "NpcStunChanceOnDamage") settings.npcStunChanceOnDamage = std::stoi(value);
                else if (key == "NpcStunDuration") settings.npcStunDuration = std::stof(value);
                else if (key == "FistDamage") settings.fistDamage = std::stoi(value);
                else if (key == "PistolDamage") settings.pistolDamage = std::stoi(value);
                else if (key == "FistVolume") settings.fistVolume = std::stof(value);
                else if (key == "PistolVolume") settings.pistolVolume = std::stof(value);
                else if (key == "TaserDamage") settings.taserDamage = std::stoi(value);
                else if (key == "TaserCooldown") settings.taserCooldown = std::stof(value);
                else if (key == "TaserRange") settings.taserRange = std::stof(value);
                else if (key == "TaserVolume") settings.taserVolume = std::stof(value);
                else if (key == "MacheteDamage") settings.macheteDamage = std::stoi(value);
                else if (key == "MacheteVolume") settings.macheteVolume = std::stof(value);
                else if (key == "KnifeDamage") settings.knifeDamage = std::stoi(value);
                else if (key == "KnifeVolume") settings.knifeVolume = std::stof(value);
                else if (key == "CrowbarDamage") settings.crowbarDamage = std::stoi(value);
                else if (key == "CrowbarVolume") settings.crowbarVolume = std::stof(value);
                else if (key == "BatDamage") settings.batDamage = std::stoi(value);
                else if (key == "BatVolume") settings.batVolume = std::stof(value);
                else if (key == "ShankDamage") settings.shankDamage = std::stoi(value);
                else if (key == "ShankVolume") settings.shankVolume = std::stof(value);
                else if (key == "BatonDamage") settings.batonDamage = std::stoi(value);
                else if (key == "BatonVolume") settings.batonVolume = std::stof(value);
                // Weapon Spawn Chances
                else if (key == "GuardPistolChance") settings.guardPistolChance = std::stoi(value);
                else if (key == "GuardAutomaticChance") settings.guardAutomaticChance = std::stoi(value);
                else if (key == "GuardTaserChance") settings.guardTaserChance = std::stoi(value);
                else if (key == "PrisonerPistolChance") settings.prisonerPistolChance = std::stoi(value);
                else if (key == "WorldSize") settings.worldSize = std::stof(value);
                else if (key == "WallCount") settings.wallCount = std::stoi(value);
            } catch (const std::exception& e) {
                 logError("Error parsing config.ini key '" + key + "': " + e.what());
            }
        }
    }
}

void SoundEngine::loadSounds() {
    const std::map<std::string, std::string> soundFileCategories = {
        {"automatic", "combat"},
        {"battle_cry", "alarm"},
        {"boss_hit", "combat"},
        {"death", "voice"},
        {"footstep", "environment"},
        {"hit", "combat"},
        {"miss", "combat"},
        {"pistol", "combat"},
        {"player_death", "player"},
        {"player_hit", "player"},
        {"punch", "combat"},
        {"reaction_panic", "alarm"},
        {"sniper", "combat"},
        {"sonar", "stealth"},
        {"takedown", "stealth"},
        {"ultimate", "combat"}
    };

    for (const auto& pair : soundFileCategories) {
        const std::string& name = pair.first;
        const std::string& category = pair.second;
        std::string path = "sounds/" + category + "/" + name + ".ogg";
        if (!soundBuffers[name].loadFromFile(path)) {
            // Fallback to root sounds directory for compatibility
            std::string fallbackPath = "sounds/" + name + ".ogg";
            if (!soundBuffers[name].loadFromFile(fallbackPath)) {
                 throw std::runtime_error("Не удалось загрузить звук: " + path + " или " + fallbackPath);
            }
        }
    }
}

// ВЕРСИЯ ДЛЯ SFML 3.0
void SoundEngine::generateSounds() {
    std::vector<std::int16_t> samples;
    bool success;
    samples.assign(44100 / 10, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(15000.0f * sin(2*3.14159f*600.0f*t) * exp(-t*30.0f)); } success = soundBuffers["DetectionTick"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate DetectionTick");
    samples.assign(44100 / 2, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(32000.0f * sin(2*3.14159f*440.0f*t) * (1.0f-t*2.0f)); } success = soundBuffers["Spotted"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate Spotted");
    samples.assign(44100, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; float pulse = (sin(2*3.14159f*2.0f*t)+1.0f)/2.0f; samples[i] = static_cast<std::int16_t>(20000.0f * sin(2*3.14159f*300.0f*t) * pulse); } success = soundBuffers["LowHealth"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate LowHealth");
    samples.assign(44100/5, 0); for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(32000.0f * sin(2*3.14159f*900.0f*t) * exp(-t*20.0f)); } success = soundBuffers["sonar"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate Sonar");
    samples.assign(44100/30, 0);for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(18000.0f * sin(2*3.14159f*1200.0f*t) * exp(-t*80.0f)); } success = soundBuffers["sonar_echo"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate sonar_echo");
    samples.assign(44100/20, 0); for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(25000.0f*sin(2*3.14159f*880.0f*t)*exp(-t*50.0f)); } success = soundBuffers["HealthIndicator"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate HealthIndicator");
    samples.assign(44100/15, 0); for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(20000.0f*sin(2*3.14159f*600.0f*t)*exp(-t*40.0f)); } success = soundBuffers["MenuSelect"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if(!success) throw std::runtime_error("Failed to generate MenuSelect");
    samples.assign(44100/10, 0); for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(22000.0f*sin(2*3.14159f*440.0f*t)*exp(-t*30.0f)); } success = soundBuffers["MenuConfirm"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if(!success) throw std::runtime_error("Failed to generate MenuConfirm");
    samples.assign(44100/8, 0); for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; float freq = 800.0f - sin(t * 3.14159f * 4.0f) * 400.0f; samples[i] = static_cast<std::int16_t>(28000.0f * sin(2*3.14159f*freq*t) * exp(-t*20.0f)); } success = soundBuffers["Stun"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if(!success) throw std::runtime_error("Failed to generate Stun");

    // --- New Weapon Sounds ---
    // Taser Fire
    samples.assign(44100 / 4, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i) / 44100.0f; float noise = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f; samples[i] = static_cast<std::int16_t>(25000.0f * (sin(2 * 3.14159f * 1200.0f * t) + 0.5f * noise) * exp(-t * 15.0f)); } success = soundBuffers["Taser_Fire"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate Taser_Fire");
    // Generic Blade Swish (for Shank/Knife)
    samples.assign(44100 / 7, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i) / 44100.0f; float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f; float envelope = exp(-t * 40.0f); samples[i] = static_cast<std::int16_t>(30000.0f * noise * envelope); } success = soundBuffers["Knife_Swish"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate Knife_Swish");
    // Machete Swish (heavier)
    samples.assign(44100 / 4, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i) / 44100.0f; float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f; float envelope = exp(-t * 20.0f); samples[i] = static_cast<std::int16_t>(25000.0f * noise * envelope); } success = soundBuffers["Machete_Swish"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate Machete_Swish");
    // Bat Swish (classic whoosh)
    samples.assign(44100 / 5, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i) / 44100.0f; float freq = 600.0f - t * 3000.0f; samples[i] = static_cast<std::int16_t>(22000.0f * sin(2 * 3.14159f * freq * t) * exp(-t * 20.0f)); } success = soundBuffers["Bat_Swish"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate Bat_Swish");
    // Crowbar/Baton Swish (more metallic)
    samples.assign(44100 / 6, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i) / 44100.0f; float freq = 800.0f - t * 2500.0f; samples[i] = static_cast<std::int16_t>(20000.0f * (sin(2 * 3.14159f * freq * t) + 0.2f * sin(2 * 3.14159f * freq * 2.5f * t)) * exp(-t * 30.0f)); } success = soundBuffers["Blunt_Metal_Swish"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate Blunt_Metal_Swish");

    // --- New Ambiance Sounds ---
    // Shadow Ambiance (subtle rustle/wind)
    samples.assign(44100, 0); for (size_t i = 0; i < samples.size(); ++i) { samples[i] = static_cast<std::int16_t>((static_cast<float>(rand()) / RAND_MAX - 0.5f) * 4000.0f); } success = soundBuffers["Shadow_Ambience"].loadFromSamples(samples.data(), samples.size(), 1, 44100, {sf::SoundChannel::Mono}); if (!success) throw std::runtime_error("Failed to generate Shadow_Ambience");
}

void SoundEngine::run() {
    logError("DEBUG_LOG: run() started.");
    window.setVerticalSyncEnabled(true);
    resetGame(); // Initialize the game state and level
    logError("DEBUG_LOG: resetGame() finished, starting main loop.");

    while (window.isOpen()) {
        float deltaTime = deltaClock.restart().asSeconds();
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        processEvents();
        if (gameState == GameState::Playing) {
            bool isMoving = processInput(deltaTime);
            player->update(deltaTime, settings, enemies);
            update(deltaTime);
            static sf::Clock stepClock;
            float stepInterval = player->isCrouching ? Player::CROUCH_STEP_INTERVAL : (player->isRunning ? Player::RUN_STEP_INTERVAL : Player::WALK_STEP_INTERVAL);
            if (isMoving && stepClock.getElapsedTime().asSeconds() > stepInterval) {
                float volume = player->isCrouching ? 40.f : (player->isRunning ? 100.f : 80.f);
                playSound("footstep", {0,0,0}, volume, true);

                // --- Новая логика шума для стелс-системы ---
                float noiseRadius = 0.0f;
                if (player->isRunning) {
                    noiseRadius = 25.0f; // Бег очень шумный
                } else if (player->isCrouching) {
                    noiseRadius = 3.0f; // Красться почти бесшумно
                } else {
                    noiseRadius = 10.0f; // Ходьба
                }
                StealthSystem::processPlayerNoise(*player, enemies, noiseRadius);
                // --- Конец новой логики ---

                stepClock.restart();
            }
        } else if (gameState == GameState::PlayerDying) {
            if (audioInitialized && playerDeathSound.getStatus() != sf::Sound::Status::Playing) {
                gameState = GameState::GameOver;
                lowHealthSound.stop(); detectionTickSound.stop();
            } else if (!audioInitialized) {
                // If there's no audio, just switch to game over after a short delay
                static sf::Clock deathTimer;
                if (deathTimer.getElapsedTime().asSeconds() > 2.0f) {
                     gameState = GameState::GameOver;
                }
            }
        }
        render();
        sf::sleep(sf::milliseconds(10)); // Increased sleep time to prevent race condition with test script
    }
}


void SoundEngine::activateDirectionalSonar(int numpadKey) {
    if (sonarClock.getElapsedTime().asSeconds() < 0.5f) return;
    sonarClock.restart();
    playSound("sonar", player->position, 40.f, false, 1.2f);
    float angle = 0.0f;
    const float PI = 3.14159f;
    switch(numpadKey) {
        case 8: angle = -PI / 2.0f; break;
        case 9: angle = -PI / 4.0f; break;
        case 6: angle = 0.0f; break;
        case 3: angle = PI / 4.0f; break;
        case 2: angle = PI / 2.0f; break;
        case 1: angle = 3.0f * PI / 4.0f; break;
        case 4: angle = PI; break;
        case 7: angle = -3.0f * PI / 4.0f; break;
    }
    sf::Vector2f rayDir(std::cos(angle), std::sin(angle));
    sf::Vector2f playerPos(player->position.x, player->position.z);
    sf::Vector2f closestIntersection = {-1, -1};
    float min_dist_sq = std::numeric_limits<float>::max();
    for (const auto& wall : walls) {
        sf::Vector2f intersection = rayIntersectsRect(playerPos, rayDir, wall);
        if (intersection.x != -1) {
            float dx = intersection.x - playerPos.x;
            float dy = intersection.y - playerPos.y;
            float dist_sq = dx * dx + dy * dy;
            if (dist_sq < min_dist_sq) {
                min_dist_sq = dist_sq;
                closestIntersection = intersection;
            }
        }
    }
    if (closestIntersection.x != -1) {
         float dist = std::sqrt(min_dist_sq);
         if (dist < 60.0f) {
             float volume = 100.0f;
             float pitch = 1.8f - (dist / 60.0f);
             playSound("sonar_echo", {closestIntersection.x, 0, closestIntersection.y}, volume, false, pitch);
         }
    }
}

bool SoundEngine::hasLineOfSightTo(const sf::Vector3f& target) {
    sf::Vector2f start(player->position.x, player->position.z);
    sf::Vector2f end(target.x, target.z);
    sf::Vector2f dir = end - start;
    float distance = std::hypot(dir.x, dir.y);
    if (distance > 0) dir /= distance;
    for(const auto& wall : walls) {
        sf::Vector2f intersection_point = rayIntersectsRect(start, dir, wall);
        if (intersection_point.x != -1) {
            float intersection_dist = std::hypot(intersection_point.x - start.x, intersection_point.y - start.y);
            if (intersection_dist < distance) {
                return false;
            }
        }
    }
    return true;
}

// ВЕРСИЯ ДЛЯ SFML 3.0
void SoundEngine::processEvents() {
    while (auto event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (gameState == GameState::Playing) {
                switch (keyPressed->scancode) {
                    case sf::Keyboard::Scan::Numpad8: activateDirectionalSonar(8); break;
                    case sf::Keyboard::Scan::Numpad9: activateDirectionalSonar(9); break;
                    case sf::Keyboard::Scan::Numpad6: activateDirectionalSonar(6); break;
                    case sf::Keyboard::Scan::Numpad3: activateDirectionalSonar(3); break;
                    case sf::Keyboard::Scan::Numpad2: activateDirectionalSonar(2); break;
                    case sf::Keyboard::Scan::Numpad1: activateDirectionalSonar(1); break;
                    case sf::Keyboard::Scan::Numpad4: activateDirectionalSonar(4); break;
                    case sf::Keyboard::Scan::Numpad7: activateDirectionalSonar(7); break;
                    default: break;
                }
                switch (keyPressed->code) {
                    case sf::Keyboard::Key::LShift:
                    case sf::Keyboard::Key::RShift:
                        player->dodge();
                        break;
                    case sf::Keyboard::Key::Escape: window.close(); break;
                    case sf::Keyboard::Key::LControl: player->toggleCrouch(); break;
                    case sf::Keyboard::Key::Num1: player->switchWeapon(WeaponType::FIST); break;
                    case sf::Keyboard::Key::Num2: player->switchWeapon(WeaponType::PISTOL); break;
                    case sf::Keyboard::Key::Num3: player->switchWeapon(WeaponType::TASER); break;
                    case sf::Keyboard::Key::Space: handlePlayerAttack(); break;
                    case sf::Keyboard::Key::F: handlePlayerTakedown(); break; // <-- Новая строка
                    case sf::Keyboard::Key::E: activateSonar(); break;
                    case sf::Keyboard::Key::G: player->godMode = !player->godMode; playSound("punch", {0,0,0}, 100.f, true); break;
                    default: break;
                }
            } else if (gameState == GameState::GameOver && keyPressed->code == sf::Keyboard::Key::R) {
                gameState = GameState::SelectingMode;
            }
        }
    }
}

// ВЕРСИЯ ДЛЯ SFML 3.0
bool SoundEngine::processInput(float deltaTime) {
    if (!player->isAlive || player->isStunned) return false;
    player->isRunning = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::RShift);
    if (player->isCrouching) player->isRunning = false;
    float speed = player->isCrouching ? Player::CROUCH_SPEED : (player->isRunning ? player->runSpeed : Player::WALK_SPEED);
    sf::Vector3f moveDir = {0,0,0};
    bool isMoving = false;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Up))    { moveDir.z -= 1; isMoving = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Down))  { moveDir.z += 1; isMoving = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Left))  { moveDir.x -= 1; isMoving = true; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Right)) { moveDir.x += 1; isMoving = true; }
    if (isMoving) {
        float len = std::hypot(moveDir.x, moveDir.z);
        if (len > 0) moveDir /= len;
        sf::Vector3f newPos = player->position + moveDir * speed * deltaTime;
        sf::FloatRect playerBounds({newPos.x - 0.4f, newPos.z - 0.4f}, {0.8f, 0.8f});
        if (!std::any_of(walls.begin(), walls.end(), [&](const auto& wall){ return wall.findIntersection(playerBounds); })) {
            player->setPosition(newPos);
        } else {
            sf::Vector3f oldPos = player->position;
            player->setPosition({newPos.x, oldPos.y, oldPos.z});
            playerBounds.position = {newPos.x - 0.4f, oldPos.z - 0.4f};
            if (std::any_of(walls.begin(), walls.end(), [&](const auto& wall){ return wall.findIntersection(playerBounds); })) {
                player->setPosition(oldPos);
            }
            oldPos = player->position;
            player->setPosition({oldPos.x, oldPos.y, newPos.z});
            playerBounds.position = {oldPos.x - 0.4f, newPos.z - 0.4f};
            if (std::any_of(walls.begin(), walls.end(), [&](const auto& wall){ return wall.findIntersection(playerBounds); })) {
                player->setPosition(oldPos);
            }
        }
    }
    return isMoving;
}

void SoundEngine::update(float deltaTime) {
    handleEnemyActions(deltaTime);
    updateLowHealthSound();
    updateShadowSound();
    if (gameMode == GameMode::CLASSIC_ACTION) { updateProximitySonar(); }
    if (!player->isAlive && gameState == GameState::Playing) {
        gameState = GameState::PlayerDying; playerDeathSound.play();
    }
}

void SoundEngine::updateShadowSound() {
    if (!audioInitialized) return;

    bool isInShadow = StealthSystem::isInShadow(player->position, shadowZones);

    if (isInShadow && shadowSound.getStatus() != sf::Sound::Status::Playing) {
        shadowSound.play();
    } else if (!isInShadow && shadowSound.getStatus() == sf::Sound::Status::Playing) {
        shadowSound.stop();
    }
}

// ВЕРСИЯ ДЛЯ SFML 3.0
void SoundEngine::generateLevel() {
    walls.clear();
    shadowZones.clear();

    // Добавим несколько теневых зон для примера
    shadowZones.push_back(sf::FloatRect({-50.f, -50.f}, {20.f, 80.f}));
    shadowZones.push_back(sf::FloatRect({30.f, 20.f}, {50.f, 15.f}));

    for (int i = 0; i < settings.wallCount; ++i) {
        float w = getFloat(5.0f, 20.0f);
        float h = getFloat(5.0f, 20.0f);
        float x = getFloat(-settings.worldSize + w, settings.worldSize - w);
        float z = getFloat(-settings.worldSize + h, settings.worldSize - h);
        sf::FloatRect newWall({x, z}, {w, h});
        sf::FloatRect playerStartArea({-2.f, -2.f}, {4.f, 4.f});
        if (newWall.findIntersection(playerStartArea)) {
            i--;
            continue;
        }
        walls.push_back(newWall);
    }
}

// ВЕРСИЯ ДЛЯ SFML 3.0
void SoundEngine::resetGame() {
    logError("DEBUG_LOG: resetGame() started.");
    gameState = GameState::Playing;
    player->reset(settings);
    generateLevel();
    sf::Listener::setDirection({0.f, 0.f, -1.f});
    enemies.clear();
    for (int i=0; i < INITIAL_NPC_COUNT; ++i) {
        // Make every 4th Enemy a guard, the rest are regular prisoners
        NPCType type = (i % 4 == 0) ? NPCType::GUARD : NPCType::REGULAR;
        enemies.push_back(std::make_unique<Enemy>(sf::Vector3f(), type, settings));
    }
    for (auto& enemy : enemies) {
        sf::Vector3f pos;
        do { pos = {getFloat(-settings.worldSize, settings.worldSize), 0, getFloat(-settings.worldSize, settings.worldSize)}; }
        while (std::hypot(pos.x, pos.z) < 20.0f);
        enemy->respawn(pos, settings);
    }
    gameClock.restart();
    // window.requestFocus(); // This can cause a hang in headless environments like xvfb
}

void SoundEngine::handleEnemyActions(float deltaTime) {
    for (auto& enemy : enemies) {
        if (enemy->isAlive) {
            // --- Новая логика обнаружения ---
            if (enemy->state != AIState::COMBAT) {
                StealthSystem::updateDetection(*enemy, *player, walls, shadowZones, settings, deltaTime);

                if (enemy->detectionLevel >= 100.f) {
                    onEnemySpottedPlayer(enemy.get(), true);
                }
            }

            // --- Существующая логика реакции на мертвые тела и помощь союзникам ---
            // (можно оставить или улучшить)
            if (enemy->state != AIState::COMBAT && !enemy->hasReactedToDeath) {
                 for (const auto& other_enemy : enemies) {
                    if (!other_enemy->isAlive) {
                        if (std::hypot(enemy->position.x - other_enemy->position.x, enemy->position.z - other_enemy->position.z) < 15.f && enemy->hasLineOfSight(other_enemy->position, walls)) {
                            playSound("reaction_panic", enemy->position, 100.f);
                            enemy->investigate(other_enemy->position);
                            enemy->hasReactedToDeath = true;
                            break;
                        }
                    }
                }
            }
            if (enemy->state != AIState::COMBAT) {
                for (const auto& other_enemy : enemies) {
                     if (other_enemy.get() != enemy.get() && other_enemy->isAlive && other_enemy->state == AIState::COMBAT) {
                        if (std::hypot(enemy->position.x - other_enemy->position.x, enemy->position.z - other_enemy->position.z) < 20.f) {
                           onEnemySpottedPlayer(enemy.get(), true); // Вступаем в бой, если союзник рядом дерется
                           break;
                        }
                    }
                }
            }

            // Обновляем самого врага
            enemy->update(deltaTime, *player, *this, settings, gameMode, walls, enemies);
        }
    }

    // Логика респавна
    for (auto& enemy : enemies) {
        if (enemy->canRespawn(settings.respawnTime)) {
            sf::Vector3f newPos;
            do { newPos.x = getFloat(-settings.worldSize, settings.worldSize); newPos.z = getFloat(-settings.worldSize, settings.worldSize); }
            while (std::hypot(newPos.x - player->position.x, newPos.z - player->position.z) < 25.0f);
            enemy->respawn(newPos, settings);
        }
    }
}

void SoundEngine::handlePlayerAttack() {
    if (!player->isAlive || player->isStunned) return; // Player cannot attack while stunned
    const auto& weapon = weapons.at(player->currentWeapon);
    if (player->lastAttackClock.getElapsedTime().asSeconds() < weapon.cooldown) return;
    player->lastAttackClock.restart();

    // Find the best target first
    Enemy* bestTarget = nullptr;
    float minDistance = std::numeric_limits<float>::max();
    for(auto& enemy : enemies) {
        if (enemy->isAlive && hasLineOfSightTo(enemy->position)) {
            float dist = std::hypot(player->position.x - enemy->position.x, player->position.z - enemy->position.z);
            if (dist < minDistance) {
                minDistance = dist;
                bestTarget = enemy.get();
            }
        }
    }

    playSound(weapon.soundName, {0,0,0}, weapon.volume, true);

    if (bestTarget) {
        if (player->currentWeapon == WeaponType::TASER) {
            if (minDistance < settings.taserRange) {
                bestTarget->takeDamage(0, *this, player.get(), true); // 0 damage, guaranteed stun
            }
        } else if ((player->currentWeapon == WeaponType::FIST && minDistance < 1.8f) || player->currentWeapon == WeaponType::PISTOL) {
            bestTarget->takeDamage(weapon.playerDamage, *this, player.get());
            if (!bestTarget->isAlive) bestTarget->onDeath(*this);
        } else if (player->currentWeapon == WeaponType::FIST) {
            playSound("miss", {0,0,0}, 70.f, true);
        }
    }
}

void SoundEngine::handlePlayerTakedown() {
    if (!player->isAlive || player->isStunned) return;

    // Ограничим частоту попыток, чтобы не спамить проверками
    if (player->lastAttackClock.getElapsedTime().asSeconds() < 0.5f) return;
    player->lastAttackClock.restart();

    // Находим ближайшего врага
    Enemy* target = nullptr;
    float minDistance = 1.5f; // Максимальная дистанция для тейкдауна

    for (auto& enemy : enemies) {
        if (!enemy->isAlive || enemy->state == AIState::COMBAT) continue;

        float dist = std::hypot(player->position.x - enemy->position.x, player->position.z - enemy->position.z);
        if (dist < minDistance) {
            // TODO: Проверить, находится ли игрок за спиной врага.
            // Это потребует добавления информации о направлении взгляда врага.
            // Пока что для теста будем считать, что любое близкое расстояние подходит.
            minDistance = dist;
            target = enemy.get();
        }
    }

    if (target) {
        playSound("takedown", target->position);
        target->takeDamage(1000, *this, player.get()); // Наносим огромный урон для мгновенного убийства
        if (!target->isAlive) {
            target->onDeath(*this);
        }
    }
}

void SoundEngine::activateSonar() {
    if (sonarClock.getElapsedTime().asSeconds() < 2.0f) return;
    sonarClock.restart();
    playSound("sonar", player->position, 80.f);
    const int numRays = 36;
    for (int i = 0; i < numRays; ++i) {
        float angle = (static_cast<float>(i) / numRays) * 2.0f * 3.14159f;
        sf::Vector2f rayDir(std::cos(angle), std::sin(angle));
        sf::Vector2f playerPos(player->position.x, player->position.z);
        sf::Vector2f closestIntersection = {-1, -1};
        float min_dist_sq = std::numeric_limits<float>::max();
        for (const auto& wall : walls) {
            sf::Vector2f intersection = rayIntersectsRect(playerPos, rayDir, wall);
            if (intersection.x != -1) {
                float dist_sq = (intersection.x - playerPos.x) * (intersection.x - playerPos.x) + (intersection.y - playerPos.y) * (intersection.y - playerPos.y);
                if (dist_sq < min_dist_sq) {
                    min_dist_sq = dist_sq;
                    closestIntersection = intersection;
                }
            }
        }
        if (closestIntersection.x != -1) {
             float dist = std::sqrt(min_dist_sq);
             if (dist < 40.0f) {
                 float volume = 100.0f * (1.0f - (dist / 40.0f));
                 float pitch = 1.5f - (dist / 40.0f);
                 playSound("sonar_echo", {closestIntersection.x, 0, closestIntersection.y}, volume, false, pitch);
             }
        }
    }
}

void SoundEngine::updateProximitySonar() {
    if (!audioInitialized) return;
    Enemy* closestEnemy = nullptr;
    float minDistance = std::numeric_limits<float>::max();
    for (const auto& enemy : enemies) {
        if (enemy->isAlive) {
            float dist = std::hypot(player->position.x - enemy->position.x, player->position.z - enemy->position.z);
            if (dist < minDistance) { minDistance = dist; closestEnemy = enemy.get(); }
        }
    }
    if (!closestEnemy || minDistance > 50.0f) return;
    float distanceFactor = 1.0f - (minDistance / 50.0f);
    float delay = 1.5f * (1.0f - distanceFactor * 0.95f);
    if (proximitySonarClock.getElapsedTime().asSeconds() > delay) {
        sonarSound.setRelativeToListener(true);
        sonarSound.setPosition({0,0,0});
        sonarSound.setPitch(1.0f + distanceFactor * 1.5f);
        sonarSound.setVolume(20.0f + distanceFactor * 80.0f);
        sonarSound.play();
        proximitySonarClock.restart();
    }
}

void SoundEngine::updateLowHealthSound() {
    if (!audioInitialized) return;
    bool isHealthLow = player->isAlive && (player->health <= settings.lowHealthThreshold);
    if (isHealthLow && lowHealthSound.getStatus() != sf::Sound::Status::Playing) {
        lowHealthSound.play();
    } else if (!isHealthLow && lowHealthSound.getStatus() == sf::Sound::Status::Playing) {
        lowHealthSound.stop();
    }
}

void SoundEngine::onEnemySpottedPlayer(Enemy* spottedBy, bool forceCombat) {
    if (spottedBy->state == AIState::COMBAT) return;

    spottedBy->state = AIState::COMBAT;
    spottedBy->detectionLevel = 0;

    // 70% chance to play a battle cry
    if (getInt(1, 100) <= 70) {
        playSound("battle_cry", spottedBy->position, 100.f, false, getFloat(0.9f, 1.1f));
    }

    if (audioInitialized) {
        detectionTickSound.stop();
    }
}

void SoundEngine::onEnemyDied(Enemy* deadEnemy) {
    playSound("death", deadEnemy->position);
}

void SoundEngine::playSound(const std::string& name, sf::Vector3f position, float volume, bool isListenerRelative, float pitch) {
    if (!audioInitialized) return;
    if (soundBuffers.find(name) == soundBuffers.end()) return;
    sf::Sound& sound = soundPool[currentSoundIndex];
    sound.stop();
    sound.setBuffer(soundBuffers.at(name));
    sound.setRelativeToListener(isListenerRelative);
    sound.setPosition(isListenerRelative ? sf::Vector3f(0,0,0) : position);
    sound.setVolume(volume);
    if (!isListenerRelative) {
        sound.setMinDistance(4.0f);
        sound.setAttenuation(1.5f);
    }
    sound.setPitch((pitch == -1.f) ? getFloat(0.95f, 1.05f) : pitch);
    sound.play();
    currentSoundIndex = (currentSoundIndex + 1) % SOUND_POOL_SIZE;
}

void SoundEngine::render() {
    window.clear(sf::Color(10, 10, 20)); // Dark blue background

    // --- Create a 2D view centered on the player ---
    sf::View view;
    view.setCenter({player->position.x, player->position.z});
    view.setSize({80.f, 60.f}); // Zoom level
    window.setView(view);

    // --- Draw Walls ---
    sf::RectangleShape wallShape;
    wallShape.setFillColor(sf::Color(100, 120, 140));
    for (const auto& wallRect : walls) {
        wallShape.setPosition({wallRect.position.x, wallRect.position.y});
        wallShape.setSize(wallRect.size);
        window.draw(wallShape);
    }

    // --- Draw Enemies ---
    sf::CircleShape enemyShape(0.4f);
    for (const auto& enemy : enemies) {
        if (enemy->isAlive) {
            enemyShape.setPosition({enemy->position.x - 0.4f, enemy->position.z - 0.4f});
            if (enemy->state == AIState::COMBAT) {
                enemyShape.setFillColor(sf::Color::Red);
            } else if (enemy->state == AIState::ALERT) {
                enemyShape.setFillColor(sf::Color::Yellow);
            } else {
                enemyShape.setFillColor(sf::Color(200, 200, 200));
            }
            window.draw(enemyShape);
        }
    }

    // --- Draw Player ---
    sf::CircleShape playerShape(0.4f);
    playerShape.setPosition({player->position.x - 0.4f, player->position.z - 0.4f});
    playerShape.setFillColor(sf::Color::Green);
    if (!player->isAlive) {
        playerShape.setFillColor(sf::Color(100, 100, 100));
    }
    window.draw(playerShape);

    window.display();
}

void SoundEngine::setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    std::cout.tie(NULL);
}