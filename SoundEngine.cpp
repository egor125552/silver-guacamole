#include "SoundEngine.h"
#include "Player.h"
#include "Enemy.h"
#include "utils.h"
#include "ai_definitions.h"

#include <AL/efx-presets.h>
#include <vorbis/vorbisfile.h>
#include <vector>
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
#include <cstdint>
#include <cstring>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
LPALGENEFFECTS alGenEffects = nullptr;
LPALDELETEEFFECTS alDeleteEffects = nullptr;
LPALEFFECTI alEffecti = nullptr;
LPALEFFECTF alEffectf = nullptr;
LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

float planarDistance(const sf::Vector3f& a, const sf::Vector3f& b) {
    return std::hypot(a.x - b.x, a.z - b.z);
}

ALuint loadOggMono(const std::string& path) {
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) throw std::runtime_error("Could not open Ogg file: " + path);

    OggVorbis_File vf;
    if (ov_open_callbacks(fp, &vf, nullptr, 0, OV_CALLBACKS_DEFAULT) < 0) {
        fclose(fp);
        throw std::runtime_error("ov_open_callbacks failed for: " + path);
    }

    vorbis_info* vi = ov_info(&vf, -1);
    const int channels = std::max(1, vi->channels);
    const long sampleRate = vi->rate;
    std::vector<char> decoded;
    char block[4096];

    while (true) {
        const long read = ov_read(&vf, block, sizeof(block), 0, 2, 1, nullptr);
        if (read < 0) {
            ov_clear(&vf);
            throw std::runtime_error("Error in Ogg stream: " + path);
        }
        if (read == 0) break;
        decoded.insert(decoded.end(), block, block + read);
    }

    std::vector<std::int16_t> mono;
    if (channels == 1) {
        const size_t samples = decoded.size() / sizeof(std::int16_t);
        mono.resize(samples);
        if (!decoded.empty()) std::memcpy(mono.data(), decoded.data(), samples * sizeof(std::int16_t));
    } else {
        const size_t frameBytes = sizeof(std::int16_t) * static_cast<size_t>(channels);
        const size_t frames = decoded.size() / frameBytes;
        mono.reserve(frames);
        for (size_t frame = 0; frame < frames; ++frame) {
            long sum = 0;
            for (int channel = 0; channel < channels; ++channel) {
                std::int16_t sample = 0;
                const size_t offset = frame * frameBytes + static_cast<size_t>(channel) * sizeof(std::int16_t);
                std::memcpy(&sample, decoded.data() + offset, sizeof(sample));
                sum += sample;
            }
            mono.push_back(static_cast<std::int16_t>(sum / channels));
        }
    }

    ALuint buffer = 0;
    alGenBuffers(1, &buffer);
    if (alGetError() != AL_NO_ERROR) {
        ov_clear(&vf);
        throw std::runtime_error("alGenBuffers failed for: " + path);
    }

    alBufferData(buffer, AL_FORMAT_MONO16, mono.data(), static_cast<ALsizei>(mono.size() * sizeof(std::int16_t)), static_cast<ALsizei>(sampleRate));
    const ALenum error = alGetError();
    ov_clear(&vf);
    if (error != AL_NO_ERROR) {
        alDeleteBuffers(1, &buffer);
        throw std::runtime_error("alBufferData failed for: " + path);
    }
    return buffer;
}

float weaponRange(WeaponType type, const GameSettings& settings) {
    switch (type) {
        case WeaponType::FIST: return 1.8f;
        case WeaponType::PISTOL: return 32.0f;
        case WeaponType::TASER: return settings.taserRange;
        case WeaponType::AUTOMATIC: return 28.0f;
        case WeaponType::SNIPER: return 75.0f;
        case WeaponType::MACHETE: return 2.2f;
        case WeaponType::KNIFE: return 1.7f;
        case WeaponType::CROWBAR: return 2.1f;
        case WeaponType::BAT: return 2.3f;
        case WeaponType::SHANK: return 1.6f;
        case WeaponType::BATON: return 2.0f;
    }
    return 0.0f;
}
}

void logError(const std::string& message) {
    std::ofstream logFile("error_log.txt", std::ios_base::app);
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    char timeString[26]{};
#ifdef _WIN32
    struct tm tmBuffer;
    localtime_s(&tmBuffer, &time);
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &tmBuffer);
#else
    struct tm tmBuffer;
    localtime_r(&time, &tmBuffer);
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &tmBuffer);
#endif
    logFile << "[" << timeString << "] " << message << std::endl;
}

SoundEngine::SoundEngine()
    : window(sf::VideoMode({800, 600}), "Stealth Action - Prison Yard", sf::Style::Titlebar | sf::Style::Close)
{
    setupConsole();
    loadSettings();

    try {
        InitOpenAL();
        audioInitialized = true;
        loadSounds();
        generateSounds();
        setReverbPreset(EFX_REVERB_PRESET_GENERIC);
    } catch (const std::exception& error) {
        logError("AUDIO_ERROR: " + std::string(error.what()));
        audioInitialized = false;
    }

    player = std::make_unique<Player>(settings);

    weapons[WeaponType::FIST]      = {settings.fistDamage, 0.5f, "punch", false, settings.fistVolume};
    weapons[WeaponType::PISTOL]    = {settings.pistolDamage, 0.4f, "pistol", false, settings.pistolVolume};
    weapons[WeaponType::TASER]     = {settings.taserDamage, settings.taserCooldown, "Taser_Fire", false, settings.taserVolume};
    weapons[WeaponType::AUTOMATIC] = {settings.automaticDamage, 0.12f, "automatic", true, settings.automaticVolume};
    weapons[WeaponType::SNIPER]    = {settings.sniperDamage, 1.5f, "sniper", false, settings.sniperVolume};
    weapons[WeaponType::MACHETE]   = {settings.macheteDamage, 0.7f, "Machete_Swish", false, settings.macheteVolume};
    weapons[WeaponType::KNIFE]     = {settings.knifeDamage, 0.4f, "Knife_Swish", false, settings.knifeVolume};
    weapons[WeaponType::CROWBAR]   = {settings.crowbarDamage, 0.9f, "Blunt_Metal_Swish", false, settings.crowbarVolume};
    weapons[WeaponType::BAT]       = {settings.batDamage, 0.8f, "Bat_Swish", false, settings.batVolume};
    weapons[WeaponType::SHANK]     = {settings.shankDamage, 0.3f, "Knife_Swish", false, settings.shankVolume};
    weapons[WeaponType::BATON]     = {settings.batonDamage, 0.6f, "Blunt_Metal_Swish", false, settings.batonVolume};
}

SoundEngine::~SoundEngine() {
    ShutdownOpenAL();
}

void SoundEngine::setReverbPreset(const EFXEAXREVERBPROPERTIES& preset) {
    if (!audioInitialized || !alEffecti || !alEffectf || !alAuxiliaryEffectSloti || reverbEffect == 0 || effectSlot == 0) return;

    // The game is audio-first, so room character should be clearly audible.
    // Keep the direct signal intact for localization, but make the wet tail richer and longer.
    const float wetGain = std::clamp(preset.flGain * 1.75f, 0.0f, 1.0f);
    const float reflectionsGain = std::clamp(preset.flReflectionsGain * 1.55f, 0.0f, 3.16f);
    const float lateGain = std::clamp(preset.flLateReverbGain * 1.65f, 0.0f, 10.0f);
    const float decayTime = std::clamp(preset.flDecayTime * 1.25f, 0.1f, 20.0f);

    alEffectf(reverbEffect, AL_REVERB_DENSITY, preset.flDensity);
    alEffectf(reverbEffect, AL_REVERB_DIFFUSION, preset.flDiffusion);
    alEffectf(reverbEffect, AL_REVERB_GAIN, wetGain);
    alEffectf(reverbEffect, AL_REVERB_GAINHF, preset.flGainHF);
    alEffectf(reverbEffect, AL_REVERB_DECAY_TIME, decayTime);
    alEffectf(reverbEffect, AL_REVERB_DECAY_HFRATIO, preset.flDecayHFRatio);
    alEffectf(reverbEffect, AL_REVERB_REFLECTIONS_GAIN, reflectionsGain);
    alEffectf(reverbEffect, AL_REVERB_REFLECTIONS_DELAY, preset.flReflectionsDelay);
    alEffectf(reverbEffect, AL_REVERB_LATE_REVERB_GAIN, lateGain);
    alEffectf(reverbEffect, AL_REVERB_LATE_REVERB_DELAY, preset.flLateReverbDelay);
    alEffectf(reverbEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, preset.flAirAbsorptionGainHF);
    alEffectf(reverbEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, preset.flRoomRolloffFactor);
    alEffecti(reverbEffect, AL_REVERB_DECAY_HFLIMIT, static_cast<ALint>(preset.iDecayHFLimit));
    alAuxiliaryEffectSloti(effectSlot, AL_EFFECTSLOT_EFFECT, reverbEffect);
}

void SoundEngine::InitOpenAL() {
    openalDevice = alcOpenDevice(nullptr);
    if (!openalDevice) throw std::runtime_error("Failed to open OpenAL device");

    openalContext = alcCreateContext(openalDevice, nullptr);
    if (!openalContext) throw std::runtime_error("Failed to create OpenAL context");
    if (!alcMakeContextCurrent(openalContext)) throw std::runtime_error("Failed to activate OpenAL context");

    if (alcIsExtensionPresent(openalDevice, "ALC_EXT_EFX") == AL_TRUE) {
#define LOAD_PROC(T, x) ((x) = reinterpret_cast<T>(alGetProcAddress(#x)))
        LOAD_PROC(LPALGENEFFECTS, alGenEffects);
        LOAD_PROC(LPALDELETEEFFECTS, alDeleteEffects);
        LOAD_PROC(LPALEFFECTI, alEffecti);
        LOAD_PROC(LPALEFFECTF, alEffectf);
        LOAD_PROC(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots);
        LOAD_PROC(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots);
        LOAD_PROC(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti);
#undef LOAD_PROC

        if (alGenEffects && alEffecti && alGenAuxiliaryEffectSlots) {
            alGenEffects(1, &reverbEffect);
            alEffecti(reverbEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
            alGenAuxiliaryEffectSlots(1, &effectSlot);
        }
    }

    soundSources.resize(SOUND_POOL_SIZE);
    alGenSources(static_cast<ALsizei>(soundSources.size()), soundSources.data());
    alGenSources(1, &lowHealthSoundSource);
    alGenSources(1, &shadowSoundSource);
    if (alGetError() != AL_NO_ERROR) throw std::runtime_error("Failed to create OpenAL sources");
}

void SoundEngine::ShutdownOpenAL() {
    if (!openalContext) {
        if (openalDevice) alcCloseDevice(openalDevice);
        openalDevice = nullptr;
        return;
    }

    if (!soundSources.empty()) {
        alSourceStopv(static_cast<ALsizei>(soundSources.size()), soundSources.data());
        alDeleteSources(static_cast<ALsizei>(soundSources.size()), soundSources.data());
    }
    if (lowHealthSoundSource) alDeleteSources(1, &lowHealthSoundSource);
    if (shadowSoundSource) alDeleteSources(1, &shadowSoundSource);

    std::vector<ALuint> buffers;
    for (const auto& entry : openalBuffers) buffers.push_back(entry.second);
    if (!buffers.empty()) alDeleteBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
    openalBuffers.clear();

    if (alDeleteAuxiliaryEffectSlots && effectSlot) alDeleteAuxiliaryEffectSlots(1, &effectSlot);
    if (alDeleteEffects && reverbEffect) alDeleteEffects(1, &reverbEffect);

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(openalContext);
    alcCloseDevice(openalDevice);
    openalContext = nullptr;
    openalDevice = nullptr;
}

void SoundEngine::loadSettings() {
    std::ifstream file("config.ini");
    if (!file.is_open()) {
        logError("Warning: config.ini not found. Using defaults.");
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        const auto comment = line.find(';');
        if (comment != std::string::npos) line.erase(comment);
        const auto separator = line.find('=');
        if (separator == std::string::npos) continue;
        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (key.empty() || value.empty()) continue;

        try {
            if (key == "Health") settings.playerHealth = std::stoi(value);
            else if (key == "PlayerRunSpeed") settings.playerRunSpeed = std::stof(value);
            else if (key == "HealthRegenRate") settings.healthRegenRate = std::stof(value);
            else if (key == "HealthRegenDelay") settings.healthRegenDelay = std::stof(value);
            else if (key == "LowHealthThreshold") settings.lowHealthThreshold = std::stoi(value);
            else if (key == "RegularHealth") settings.regularHealth = std::stoi(value);
            else if (key == "ShooterHealth") settings.shooterHealth = std::stoi(value);
            else if (key == "BossHealth") settings.bossHealth = std::stoi(value);
            else if (key == "MeleeNpcCanAttack") settings.meleeNpcCanAttack = value == "yes" || value == "true" || value == "1";
            else if (key == "NpcWalkSpeed") settings.npcWalkSpeed = std::stof(value);
            else if (key == "NpcRunSpeed") settings.npcRunSpeed = std::stof(value);
            else if (key == "NpcStunChanceOnDamage") settings.npcStunChanceOnDamage = std::stoi(value);
            else if (key == "NpcStunDuration") settings.npcStunDuration = std::stof(value);
            else if (key == "FistDamage") settings.fistDamage = std::stoi(value);
            else if (key == "PistolDamage") settings.pistolDamage = std::stoi(value);
            else if (key == "AutomaticDamage") settings.automaticDamage = std::stoi(value);
            else if (key == "SniperDamage") settings.sniperDamage = std::stoi(value);
            else if (key == "FistVolume") settings.fistVolume = std::stof(value);
            else if (key == "PistolVolume") settings.pistolVolume = std::stof(value);
            else if (key == "AutomaticVolume") settings.automaticVolume = std::stof(value);
            else if (key == "SniperVolume") settings.sniperVolume = std::stof(value);
            else if (key == "TaserDamage") settings.taserDamage = std::stoi(value);
            else if (key == "TaserCooldown") settings.taserCooldown = std::stof(value);
            else if (key == "TaserRange") settings.taserRange = std::clamp(std::stof(value), 1.0f, 12.0f);
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
            else if (key == "GuardPistolChance") settings.guardPistolChance = std::clamp(std::stoi(value), 0, 100);
            else if (key == "GuardAutomaticChance") settings.guardAutomaticChance = std::clamp(std::stoi(value), 0, 100);
            else if (key == "GuardTaserChance") settings.guardTaserChance = std::clamp(std::stoi(value), 0, 100);
            else if (key == "PrisonerPistolChance") settings.prisonerPistolChance = std::clamp(std::stoi(value), 0, 100);
            else if (key == "WorldSize") settings.worldSize = std::max(20.0f, std::stof(value));
            else if (key == "WallCount") settings.wallCount = std::clamp(std::stoi(value), 0, 100);
            else if (key == "GracePeriod") settings.gracePeriod = std::max(0.0f, std::stof(value));
            else if (key == "RespawnTime") settings.respawnTime = std::max(1.0f, std::stof(value));
        } catch (const std::exception& error) {
            logError("Invalid config value for " + key + ": " + error.what());
        }
    }
}

void SoundEngine::loadSounds() {
    const std::vector<std::string> names = {
        "automatic", "battle_cry", "boss_hit", "death", "footstep", "hit", "miss", "pistol",
        "player_death", "player_hit", "punch", "reaction_panic", "sniper", "sonar", "takedown", "ultimate"
    };

    for (const auto& name : names) {
        try {
            openalBuffers[name] = loadOggMono("sounds/" + name + ".ogg");
        } catch (const std::exception& error) {
            logError("AUDIO_WARNING: " + std::string(error.what()));
        }
    }

    const std::vector<std::pair<std::string, std::string>> optionalEvents = {
        {"guard_vs_lukashenko_main", "sounds/guard_vs_lukashenko/main_event.ogg"},
        {"prisoners_arguing_main", "sounds/prisoners_arguing/main_event.ogg"}
    };
    for (const auto& [name, path] : optionalEvents) {
        try {
            openalBuffers[name] = loadOggMono(path);
        } catch (const std::exception& error) {
            logError("OPTIONAL_AUDIO_WARNING: " + std::string(error.what()));
        }
    }
}

void SoundEngine::generateSounds() {
    auto createBuffer = [&](const std::string& name, const std::vector<std::int16_t>& data, int sampleRate = 44100) {
        if (openalBuffers.find(name) != openalBuffers.end()) return;
        ALuint buffer = 0;
        alGenBuffers(1, &buffer);
        alBufferData(buffer, AL_FORMAT_MONO16, data.data(), static_cast<ALsizei>(data.size() * sizeof(std::int16_t)), sampleRate);
        if (alGetError() == AL_NO_ERROR) openalBuffers[name] = buffer;
        else if (buffer) alDeleteBuffers(1, &buffer);
    };

    auto tone = [&](const std::string& name, float frequency, float seconds, float decay, float gain) {
        const size_t count = static_cast<size_t>(44100.0f * seconds);
        std::vector<std::int16_t> samples(count);
        for (size_t i = 0; i < count; ++i) {
            const float t = static_cast<float>(i) / 44100.0f;
            samples[i] = static_cast<std::int16_t>(gain * std::sin(2.0f * 3.1415926f * frequency * t) * std::exp(-t * decay));
        }
        createBuffer(name, samples);
    };

    tone("DetectionTick", 600.f, 0.10f, 30.f, 15000.f);
    tone("Spotted", 440.f, 0.45f, 3.f, 26000.f);
    tone("HealthIndicator", 880.f, 0.05f, 50.f, 25000.f);
    tone("MenuSelect", 700.f, 0.06f, 35.f, 17000.f);
    tone("MenuConfirm", 440.f, 0.10f, 25.f, 21000.f);
    tone("Dodge", 1050.f, 0.08f, 28.f, 19000.f);
    tone("Stun", 280.f, 0.22f, 8.f, 26000.f);
    tone("sonar_echo", 1200.f, 0.035f, 80.f, 18000.f);
    tone("EnemyPing", 980.f, 0.065f, 42.f, 26000.f);

    if (openalBuffers.find("LowHealth") == openalBuffers.end()) {
        std::vector<std::int16_t> samples(44100);
        for (size_t i = 0; i < samples.size(); ++i) {
            const float t = static_cast<float>(i) / 44100.0f;
            const float pulse = (std::sin(2.0f * 3.1415926f * 2.0f * t) + 1.0f) * 0.5f;
            samples[i] = static_cast<std::int16_t>(19000.f * std::sin(2.0f * 3.1415926f * 300.0f * t) * pulse);
        }
        createBuffer("LowHealth", samples);
    }

    if (openalBuffers.find("Shadow_Ambience") == openalBuffers.end()) {
        std::vector<std::int16_t> samples(44100);
        for (auto& sample : samples) sample = static_cast<std::int16_t>((static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 3500.0f);
        createBuffer("Shadow_Ambience", samples);
    }

    if (openalBuffers.find("Taser_Fire") == openalBuffers.end()) {
        std::vector<std::int16_t> samples(44100 / 4);
        for (size_t i = 0; i < samples.size(); ++i) {
            const float t = static_cast<float>(i) / 44100.0f;
            const float noise = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.f - 1.f;
            samples[i] = static_cast<std::int16_t>(23000.f * (std::sin(2.f * 3.1415926f * 1200.f * t) + 0.45f * noise) * std::exp(-t * 15.f));
        }
        createBuffer("Taser_Fire", samples);
    }

    if (openalBuffers.find("Knife_Swish") == openalBuffers.end()) tone("Knife_Swish", 920.f, 0.08f, 24.f, 17000.f);
    if (openalBuffers.find("Machete_Swish") == openalBuffers.end()) tone("Machete_Swish", 520.f, 0.18f, 15.f, 21000.f);
    if (openalBuffers.find("Bat_Swish") == openalBuffers.end()) tone("Bat_Swish", 330.f, 0.16f, 14.f, 20000.f);
    if (openalBuffers.find("Blunt_Metal_Swish") == openalBuffers.end()) tone("Blunt_Metal_Swish", 650.f, 0.13f, 20.f, 19000.f);
}

void SoundEngine::run() {
    window.setVerticalSyncEnabled(true);
    resetGame();

    while (window.isOpen()) {
        float deltaTime = std::min(deltaClock.restart().asSeconds(), 0.1f);
        processEvents();

        if (gameState == GameState::Playing) {
            const bool moving = processInput(deltaTime);
            player->update(deltaTime, settings, enemies);
            update(deltaTime);

            static sf::Clock stepClock;
            const float interval = player->isCrouching ? Player::CROUCH_STEP_INTERVAL : (player->isRunning ? Player::RUN_STEP_INTERVAL : Player::WALK_STEP_INTERVAL);
            if (moving && stepClock.getElapsedTime().asSeconds() > interval) {
                // Player footsteps are intentionally dry and listener-relative: they should always be readable
                // even when the environment and nearby enemies are loud.
                const float volume = player->isCrouching ? 72.f : (player->isRunning ? 120.f : 108.f);
                const float pitch = player->isRunning ? getFloat(1.02f, 1.08f) : getFloat(0.96f, 1.04f);
                playSound("footstep", {0, 0, 0}, volume, true, pitch);
                const float noise = player->isRunning ? 25.f : (player->isCrouching ? 3.f : 10.f);
                StealthSystem::processPlayerNoise(*player, enemies, noise);
                stepClock.restart();
            }
        } else if (gameState == GameState::PlayerDying) {
            static bool started = false;
            static sf::Clock deathTimer;
            if (!started) {
                deathTimer.restart();
                started = true;
            }
            if (deathTimer.getElapsedTime().asSeconds() > 2.5f) {
                gameState = GameState::GameOver;
                if (audioInitialized) {
                    alSourceStop(lowHealthSoundSource);
                    alSourceStop(shadowSoundSource);
                }
                started = false;
            }
        }

        render();
        sf::sleep(sf::milliseconds(8));
    }
}

void SoundEngine::processEvents() {
    while (auto event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) window.close();
        const auto* keyPressed = event->getIf<sf::Event::KeyPressed>();
        if (!keyPressed) continue;

        if (gameState == GameState::GameOver) {
            if (keyPressed->code == sf::Keyboard::Key::R) resetGame();
            else if (keyPressed->code == sf::Keyboard::Key::Escape) window.close();
            continue;
        }
        if (gameState != GameState::Playing) continue;

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

        auto selectWeapon = [&](WeaponType type, float pitch) {
            player->switchWeapon(type);
            playSound("MenuSelect", {0, 0, 0}, 75.f, true, pitch);
        };

        switch (keyPressed->code) {
            case sf::Keyboard::Key::Escape: window.close(); break;
            case sf::Keyboard::Key::LControl: player->toggleCrouch(); playSound("MenuConfirm", {0,0,0}, 55.f, true, player->isCrouching ? 0.8f : 1.2f); break;
            case sf::Keyboard::Key::X: if (player->dodge()) playSound("Dodge", {0,0,0}, 90.f, true); break;
            case sf::Keyboard::Key::Num1: selectWeapon(WeaponType::FIST, 0.70f); break;
            case sf::Keyboard::Key::Num2: selectWeapon(WeaponType::PISTOL, 0.80f); break;
            case sf::Keyboard::Key::Num3: selectWeapon(WeaponType::TASER, 0.90f); break;
            case sf::Keyboard::Key::Num4: selectWeapon(WeaponType::AUTOMATIC, 1.00f); break;
            case sf::Keyboard::Key::Num5: selectWeapon(WeaponType::SNIPER, 1.10f); break;
            case sf::Keyboard::Key::Num6: selectWeapon(WeaponType::MACHETE, 1.20f); break;
            case sf::Keyboard::Key::Num7: selectWeapon(WeaponType::KNIFE, 1.30f); break;
            case sf::Keyboard::Key::Num8: selectWeapon(WeaponType::CROWBAR, 1.40f); break;
            case sf::Keyboard::Key::Num9: selectWeapon(WeaponType::BAT, 1.50f); break;
            case sf::Keyboard::Key::Num0: selectWeapon(WeaponType::SHANK, 1.60f); break;
            case sf::Keyboard::Key::B: selectWeapon(WeaponType::BATON, 1.70f); break;
            case sf::Keyboard::Key::Space: handlePlayerAttack(); break;
            case sf::Keyboard::Key::F: handlePlayerTakedown(); break;
            case sf::Keyboard::Key::E: activateSonar(); break;
#ifndef NDEBUG
            case sf::Keyboard::Key::G: player->godMode = !player->godMode; playSound("MenuConfirm", {0,0,0}, 100.f, true, player->godMode ? 1.5f : 0.7f); break;
#endif
            default: break;
        }
    }
}

bool SoundEngine::processInput(float deltaTime) {
    if (!player->isAlive || player->isStunned) return false;

    player->isRunning = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::RShift);
    if (player->isCrouching) player->isRunning = false;
    const float speed = player->isCrouching ? Player::CROUCH_SPEED : (player->isRunning ? player->runSpeed : Player::WALK_SPEED);

    sf::Vector3f moveDir{0, 0, 0};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Up)) moveDir.z -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Down)) moveDir.z += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Left)) moveDir.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Right)) moveDir.x += 1.f;
    const bool moving = std::hypot(moveDir.x, moveDir.z) > 0.001f;

    if (moving) {
        const float length = std::hypot(moveDir.x, moveDir.z);
        moveDir /= length;
        sf::Vector3f desired = player->position + moveDir * speed * deltaTime;
        desired.x = std::clamp(desired.x, -settings.worldSize, settings.worldSize);
        desired.z = std::clamp(desired.z, -settings.worldSize, settings.worldSize);

        auto blocked = [&](const sf::Vector3f& pos) {
            const sf::FloatRect bounds({pos.x - 0.4f, pos.z - 0.4f}, {0.8f, 0.8f});
            return std::any_of(walls.begin(), walls.end(), [&](const auto& wall) { return wall.findIntersection(bounds).has_value(); });
        };

        if (!blocked(desired)) {
            player->setPosition(desired);
        } else {
            const sf::Vector3f xOnly{desired.x, player->position.y, player->position.z};
            const sf::Vector3f zOnly{player->position.x, player->position.y, desired.z};
            if (!blocked(xOnly)) player->setPosition(xOnly);
            else if (!blocked(zOnly)) player->setPosition(zOnly);
        }
    }

    const auto& current = weapons.at(player->currentWeapon);
    if (current.isAutomatic && sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space)) handlePlayerAttack();
    return moving;
}

void SoundEngine::update(float deltaTime) {
    handleEnemyActions(deltaTime);

    static bool inCave = false;
    const bool nowInCave = player->position.x > 50.f;
    if (nowInCave != inCave) {
        if (nowInCave) setReverbPreset(EFX_REVERB_PRESET_CAVE);
        else setReverbPreset(EFX_REVERB_PRESET_GENERIC);
        inCave = nowInCave;
    }

    updateLowHealthSound();
    updateShadowSound();
    updateProximitySonar();

    if (!player->isAlive && gameState == GameState::Playing) {
        gameState = GameState::PlayerDying;
        playSound("player_death", {0, 0, 0}, 100.f, true);
    }
}

void SoundEngine::generateLevel() {
    walls.clear();
    shadowZones.clear();
    shadowZones.push_back(sf::FloatRect({-50.f, -50.f}, {20.f, 80.f}));
    shadowZones.push_back(sf::FloatRect({30.f, 20.f}, {50.f, 15.f}));

    int attempts = 0;
    while (static_cast<int>(walls.size()) < settings.wallCount && attempts < settings.wallCount * 20 + 100) {
        ++attempts;
        const float width = getFloat(5.0f, 20.0f);
        const float height = getFloat(5.0f, 20.0f);
        const float x = getFloat(-settings.worldSize + width, settings.worldSize - width);
        const float z = getFloat(-settings.worldSize + height, settings.worldSize - height);
        sf::FloatRect wall({x, z}, {width, height});
        const sf::FloatRect startArea({-5.f, -5.f}, {10.f, 10.f});
        if (wall.findIntersection(startArea)) continue;

        bool severeOverlap = false;
        for (const auto& existing : walls) {
            const auto intersection = wall.findIntersection(existing);
            if (intersection && intersection->size.x * intersection->size.y > width * height * 0.45f) {
                severeOverlap = true;
                break;
            }
        }
        if (!severeOverlap) walls.push_back(wall);
    }
}

void SoundEngine::resetGame() {
    gameState = GameState::Playing;
    player->reset(settings);
    generateLevel();
    if (audioInitialized) alListener3f(AL_DIRECTION, 0.f, 0.f, -1.f);

    auto blocked = [&](const sf::Vector3f& pos) {
        const sf::FloatRect bounds({pos.x - 0.5f, pos.z - 0.5f}, {1.f, 1.f});
        return std::any_of(walls.begin(), walls.end(), [&](const auto& wall) { return wall.findIntersection(bounds).has_value(); });
    };

    auto safeSpawn = [&](float minPlayerDistance) {
        sf::Vector3f pos{};
        for (int attempt = 0; attempt < 500; ++attempt) {
            pos = {getFloat(-settings.worldSize + 1.f, settings.worldSize - 1.f), 0.f, getFloat(-settings.worldSize + 1.f, settings.worldSize - 1.f)};
            if (planarDistance(pos, player->position) >= minPlayerDistance && !blocked(pos)) return pos;
        }
        return sf::Vector3f{settings.worldSize * 0.75f, 0.f, settings.worldSize * 0.75f};
    };

    enemies.clear();
    for (int i = 0; i < INITIAL_NPC_COUNT; ++i) {
        NPCType type = (i % 4 == 0) ? NPCType::GUARD : NPCType::REGULAR;
        if (i == INITIAL_NPC_COUNT - 2) type = NPCType::SHOOTER;
        enemies.push_back(std::make_unique<Enemy>(safeSpawn(22.f), type, settings));
    }
    gameClock.restart();
    sonarClock.restart();
    proximitySonarClock.restart();
}

void SoundEngine::handleEnemyActions(float deltaTime) {
    if (gameClock.getElapsedTime().asSeconds() < settings.gracePeriod) return;

    for (auto& enemy : enemies) {
        if (!enemy->isAlive) continue;

        if (enemy->state != AIState::COMBAT) {
            StealthSystem::updateDetection(*enemy, *player, walls, shadowZones, settings, deltaTime);
            if (enemy->detectionLevel >= 100.f) onEnemySpottedPlayer(enemy.get(), true);
        }

        if (enemy->state != AIState::COMBAT && !enemy->hasReactedToDeath) {
            for (const auto& other : enemies) {
                if (!other->isAlive && planarDistance(enemy->position, other->position) < 15.f && enemy->hasLineOfSight(other->position, walls)) {
                    playSound("reaction_panic", enemy->position, 100.f);
                    enemy->investigate(other->position);
                    enemy->hasReactedToDeath = true;
                    break;
                }
            }
        }

        if (enemy->state != AIState::COMBAT) {
            for (const auto& other : enemies) {
                if (other.get() == enemy.get() || !other->isAlive || other->state != AIState::COMBAT) continue;
                if (planarDistance(enemy->position, other->position) < 20.f && enemy->hasLineOfSight(other->position, walls)) {
                    onEnemySpottedPlayer(enemy.get(), true);
                    break;
                }
            }
        }

        enemy->update(deltaTime, *player, *this, settings, gameMode, walls, enemies);
    }

    auto blocked = [&](const sf::Vector3f& pos) {
        const sf::FloatRect bounds({pos.x - 0.5f, pos.z - 0.5f}, {1.f, 1.f});
        return std::any_of(walls.begin(), walls.end(), [&](const auto& wall) { return wall.findIntersection(bounds).has_value(); });
    };

    for (auto& enemy : enemies) {
        if (!enemy->canRespawn(settings.respawnTime)) continue;
        sf::Vector3f newPos{};
        bool found = false;
        for (int attempt = 0; attempt < 300; ++attempt) {
            newPos = {getFloat(-settings.worldSize + 1.f, settings.worldSize - 1.f), 0.f, getFloat(-settings.worldSize + 1.f, settings.worldSize - 1.f)};
            if (planarDistance(newPos, player->position) >= 25.f && !blocked(newPos)) {
                found = true;
                break;
            }
        }
        if (found) enemy->respawn(newPos, settings);
    }
}

void SoundEngine::handlePlayerAttack() {
    if (!player->isAlive || player->isStunned) return;
    const auto& weapon = weapons.at(player->currentWeapon);
    if (player->lastAttackClock.getElapsedTime().asSeconds() < weapon.cooldown) return;
    player->lastAttackClock.restart();

    const float range = weaponRange(player->currentWeapon, settings);
    Enemy* target = nullptr;
    float closest = range;
    for (auto& enemy : enemies) {
        if (!enemy->isAlive || !hasLineOfSightTo(enemy->position)) continue;
        const float distance = planarDistance(player->position, enemy->position);
        if (distance <= closest) {
            closest = distance;
            target = enemy.get();
        }
    }

    playSound(weapon.soundName, {0, 0, 0}, weapon.volume, true);
    if (!target) {
        if (player->currentWeapon == WeaponType::FIST || player->currentWeapon == WeaponType::MACHETE || player->currentWeapon == WeaponType::KNIFE || player->currentWeapon == WeaponType::CROWBAR || player->currentWeapon == WeaponType::BAT || player->currentWeapon == WeaponType::SHANK || player->currentWeapon == WeaponType::BATON) {
            playSound("miss", {0,0,0}, 55.f, true);
        }
        return;
    }

    if (player->currentWeapon == WeaponType::TASER) target->takeDamage(0, *this, player.get(), true);
    else target->takeDamage(weapon.playerDamage, *this, player.get());
}

void SoundEngine::handlePlayerTakedown() {
    if (!player->isAlive || player->isStunned) return;
    if (player->lastAttackClock.getElapsedTime().asSeconds() < 0.8f) return;

    Enemy* target = nullptr;
    float closest = 1.5f;
    for (auto& enemy : enemies) {
        if (!enemy->isAlive || enemy->state != AIState::PATROLLING || enemy->detectionLevel >= 60.f) continue;
        const float distance = planarDistance(player->position, enemy->position);
        if (distance < closest && hasLineOfSightTo(enemy->position)) {
            closest = distance;
            target = enemy.get();
        }
    }

    player->lastAttackClock.restart();
    if (!target) {
        playSound("miss", {0,0,0}, 45.f, true);
        return;
    }

    playSound("takedown", target->position);
    target->takeDamage(target->health + 1, *this, player.get());
}

void SoundEngine::activateSonar() {
    if (sonarClock.getElapsedTime().asSeconds() < 2.0f) return;
    sonarClock.restart();
    playSound("sonar", {0,0,0}, 80.f, true);

    constexpr int rays = 36;
    for (int index = 0; index < rays; ++index) {
        const float angle = static_cast<float>(index) / rays * 2.f * 3.1415926f;
        const sf::Vector2f direction(std::cos(angle), std::sin(angle));
        const sf::Vector2f origin(player->position.x, player->position.z);
        sf::Vector2f closest{-1.f, -1.f};
        float best = std::numeric_limits<float>::max();
        for (const auto& wall : walls) {
            const sf::Vector2f hit = rayIntersectsRect(origin, direction, wall);
            if (hit.x == -1) continue;
            const float distanceSquared = (hit.x - origin.x) * (hit.x - origin.x) + (hit.y - origin.y) * (hit.y - origin.y);
            if (distanceSquared < best) {
                best = distanceSquared;
                closest = hit;
            }
        }
        if (closest.x != -1) {
            const float distance = std::sqrt(best);
            if (distance < 40.f) {
                playSound("sonar_echo", {closest.x, 0, closest.y}, 100.f * (1.f - distance / 40.f), false, 1.5f - distance / 40.f);
            }
        }
    }
}

void SoundEngine::activateDirectionalSonar(int numpadKey) {
    if (sonarClock.getElapsedTime().asSeconds() < 0.5f) return;
    sonarClock.restart();
    playSound("sonar", {0,0,0}, 40.f, true, 1.2f);

    const float pi = 3.1415926f;
    float angle = 0.f;
    switch (numpadKey) {
        case 8: angle = -pi / 2.f; break;
        case 9: angle = -pi / 4.f; break;
        case 6: angle = 0.f; break;
        case 3: angle = pi / 4.f; break;
        case 2: angle = pi / 2.f; break;
        case 1: angle = 3.f * pi / 4.f; break;
        case 4: angle = pi; break;
        case 7: angle = -3.f * pi / 4.f; break;
        default: return;
    }

    const sf::Vector2f direction(std::cos(angle), std::sin(angle));
    const sf::Vector2f origin(player->position.x, player->position.z);
    sf::Vector2f closest{-1.f, -1.f};
    float best = std::numeric_limits<float>::max();
    for (const auto& wall : walls) {
        const sf::Vector2f hit = rayIntersectsRect(origin, direction, wall);
        if (hit.x == -1) continue;
        const float d2 = (hit.x - origin.x) * (hit.x - origin.x) + (hit.y - origin.y) * (hit.y - origin.y);
        if (d2 < best) { best = d2; closest = hit; }
    }
    if (closest.x != -1) {
        const float distance = std::sqrt(best);
        if (distance < 60.f) playSound("sonar_echo", {closest.x, 0, closest.y}, 100.f, false, 1.8f - distance / 60.f);
    }
}

void SoundEngine::updateProximitySonar() {
    if (!audioInitialized) return;

    Enemy* closestEnemy = nullptr;
    float closest = std::numeric_limits<float>::max();
    for (const auto& enemy : enemies) {
        if (!enemy->isAlive) continue;
        const float distance = planarDistance(player->position, enemy->position);
        if (distance < closest) {
            closest = distance;
            closestEnemy = enemy.get();
        }
    }
    if (!closestEnemy || closest > 50.f) return;

    const float factor = 1.f - closest / 50.f;
    const float delay = std::max(0.14f, 1.45f * (1.f - factor * 0.91f));
    if (proximitySonarClock.getElapsedTime().asSeconds() > delay) {
        // The proximity cue comes from the enemy itself. This turns the ping into a stereo locator:
        // left enemy = left ping, right enemy = right ping, with pitch/rate still conveying distance.
        const float volume = 72.f + factor * 45.f;
        const float pitch = 0.92f + factor * 0.72f;
        playSound("EnemyPing", closestEnemy->position, volume, false, pitch);
        proximitySonarClock.restart();
    }
}

void SoundEngine::updateLowHealthSound() {
    if (!audioInitialized || openalBuffers.find("LowHealth") == openalBuffers.end()) return;
    const bool low = player->isAlive && player->health <= settings.lowHealthThreshold;
    ALint state = AL_STOPPED;
    alGetSourcei(lowHealthSoundSource, AL_SOURCE_STATE, &state);
    if (low && state != AL_PLAYING) {
        alSourcei(lowHealthSoundSource, AL_BUFFER, openalBuffers.at("LowHealth"));
        alSourcef(lowHealthSoundSource, AL_GAIN, 0.8f);
        alSourcei(lowHealthSoundSource, AL_SOURCE_RELATIVE, AL_TRUE);
        alSourcei(lowHealthSoundSource, AL_LOOPING, AL_TRUE);
        alSourcePlay(lowHealthSoundSource);
    } else if (!low && state == AL_PLAYING) {
        alSourceStop(lowHealthSoundSource);
        alSourcei(lowHealthSoundSource, AL_BUFFER, 0);
    }
}

void SoundEngine::updateShadowSound() {
    if (!audioInitialized || openalBuffers.find("Shadow_Ambience") == openalBuffers.end()) return;
    const bool shadow = StealthSystem::isInShadow(player->position, shadowZones);
    ALint state = AL_STOPPED;
    alGetSourcei(shadowSoundSource, AL_SOURCE_STATE, &state);
    if (shadow && state != AL_PLAYING) {
        alSourcei(shadowSoundSource, AL_BUFFER, openalBuffers.at("Shadow_Ambience"));
        alSourcef(shadowSoundSource, AL_GAIN, 0.30f);
        alSourcei(shadowSoundSource, AL_SOURCE_RELATIVE, AL_TRUE);
        alSourcei(shadowSoundSource, AL_LOOPING, AL_TRUE);
        alSourcePlay(shadowSoundSource);
    } else if (!shadow && state == AL_PLAYING) {
        alSourceStop(shadowSoundSource);
        alSourcei(shadowSoundSource, AL_BUFFER, 0);
    }
}

void SoundEngine::onEnemySpottedPlayer(Enemy* spottedBy, bool forceCombat) {
    (void)forceCombat;
    if (!spottedBy || spottedBy->state == AIState::COMBAT) return;
    spottedBy->state = AIState::COMBAT;
    spottedBy->detectionLevel = 0.f;
    playSound("Spotted", spottedBy->position, 85.f);
    if (getInt(1, 100) <= 70) playSound("battle_cry", spottedBy->position, 100.f, false, getFloat(0.9f, 1.1f));
}

void SoundEngine::onEnemyDied(Enemy* deadEnemy) {
    if (deadEnemy) playSound("death", deadEnemy->position);
}

bool SoundEngine::hasLineOfSightTo(const sf::Vector3f& target) {
    const sf::Vector2f start(player->position.x, player->position.z);
    const sf::Vector2f end(target.x, target.z);
    sf::Vector2f direction = end - start;
    const float distance = std::hypot(direction.x, direction.y);
    if (distance > 0) direction /= distance;
    for (const auto& wall : walls) {
        const sf::Vector2f hit = rayIntersectsRect(start, direction, wall);
        if (hit.x == -1) continue;
        if (std::hypot(hit.x - start.x, hit.y - start.y) < distance) return false;
    }
    return true;
}

void SoundEngine::playSound(const std::string& name, sf::Vector3f position, float volume, bool isListenerRelative, float pitch) {
    logError("DEBUG_LOG: Playing sound " + name);
    if (!audioInitialized || soundSources.empty()) return;
    const auto it = openalBuffers.find(name);
    if (it == openalBuffers.end()) {
        logError("AUDIO_WARNING: Missing sound buffer " + name);
        return;
    }

    ALuint source = soundSources[currentSourceIndex];
    alSourceStop(source);
    alSourcei(source, AL_BUFFER, 0);
    alSourcei(source, AL_BUFFER, it->second);
    alSourcef(source, AL_PITCH, pitch < 0.f ? getFloat(0.97f, 1.03f) : pitch);
    alSourcef(source, AL_GAIN, std::clamp(volume / 100.f, 0.f, 1.25f));
    alSourcei(source, AL_SOURCE_RELATIVE, isListenerRelative ? AL_TRUE : AL_FALSE);

    if (isListenerRelative) {
        alSource3f(source, AL_POSITION, 0, 0, 0);
        alSource3i(source, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
    } else {
        alSource3f(source, AL_POSITION, position.x, position.y, position.z);
        if (name == "EnemyPing") {
            // Keep the enemy locator readable all the way to the 50 m proximity-sonar radius.
            alSourcef(source, AL_REFERENCE_DISTANCE, 11.f);
            alSourcef(source, AL_ROLLOFF_FACTOR, 0.62f);
            alSourcef(source, AL_MAX_DISTANCE, 60.f);
        } else {
            alSourcef(source, AL_REFERENCE_DISTANCE, 4.f);
            alSourcef(source, AL_ROLLOFF_FACTOR, 1.5f);
        }
        if (effectSlot) alSource3i(source, AL_AUXILIARY_SEND_FILTER, effectSlot, 0, AL_FILTER_NULL);
    }

    if (alGetError() == AL_NO_ERROR) alSourcePlay(source);
    currentSourceIndex = (currentSourceIndex + 1) % soundSources.size();
}

void SoundEngine::render() {
    window.clear(sf::Color(10, 10, 20));
    sf::View view;
    view.setCenter({player->position.x, player->position.z});
    view.setSize({80.f, 60.f});
    window.setView(view);

    sf::RectangleShape wallShape;
    wallShape.setFillColor(sf::Color(100, 120, 140));
    for (const auto& wall : walls) {
        wallShape.setPosition({wall.position.x, wall.position.y});
        wallShape.setSize(wall.size);
        window.draw(wallShape);
    }

    sf::CircleShape enemyShape(0.4f);
    for (const auto& enemy : enemies) {
        if (!enemy->isAlive) continue;
        enemyShape.setPosition({enemy->position.x - 0.4f, enemy->position.z - 0.4f});
        enemyShape.setFillColor(enemy->state == AIState::COMBAT ? sf::Color::Red : (enemy->state == AIState::ALERT || enemy->state == AIState::SEARCHING ? sf::Color::Yellow : sf::Color(200, 200, 200)));
        window.draw(enemyShape);
    }

    sf::CircleShape playerShape(0.4f);
    playerShape.setPosition({player->position.x - 0.4f, player->position.z - 0.4f});
    playerShape.setFillColor(player->isAlive ? sf::Color::Green : sf::Color(100, 100, 100));
    window.draw(playerShape);
    window.display();
}

void SoundEngine::setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
}

void SoundEngine::_test_setPlayerPosition(const sf::Vector3f& pos) {
    player->setPosition(pos);
}

void SoundEngine::_test_setEnemies(std::vector<std::unique_ptr<Enemy>>&& newEnemies) {
    enemies = std::move(newEnemies);
}
