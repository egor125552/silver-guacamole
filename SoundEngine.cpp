#include "SoundEngine.h"
#include "Player.h"
#include "Enemy.h"

#include <AL/efx-presets.h>
#include <vorbis/vorbisfile.h>
#include <vector>
#include <iostream>

// Определения и указатели на функции EFX
namespace {
    // Effect objects
    LPALGENEFFECTS alGenEffects = nullptr;
    LPALDELETEEFFECTS alDeleteEffects = nullptr;
    LPALISEFFECT alIsEffect = nullptr;
    LPALEFFECTI alEffecti = nullptr;
    LPALEFFECTIV alEffectiv = nullptr;
    LPALEFFECTF alEffectf = nullptr;
    LPALEFFECTFV alEffectfv = nullptr;
    LPALGETEFFECTI alGetEffecti = nullptr;
    LPALGETEFFECTIV alGetEffectiv = nullptr;
    LPALGETEFFECTF alGetEffectf = nullptr;
    LPALGETEFFECTFV alGetEffectfv = nullptr;

    // Filter objects
    LPALGENFILTERS alGenFilters = nullptr;
    LPALDELETEFILTERS alDeleteFilters = nullptr;
    LPALISFILTER alIsFilter = nullptr;
    LPALFILTERI alFilteri = nullptr;
    LPALFILTERIV alFilteriv = nullptr;
    LPALFILTERF alFilterf = nullptr;
    LPALFILTERFV alFilterfv = nullptr;
    LPALGETFILTERI alGetFilteri = nullptr;
    LPALGETFILTERIV alGetFilteriv = nullptr;
    LPALGETFILTERF alGetFilterf = nullptr;
    LPALGETFILTERFV alGetFilterfv = nullptr;

    // Auxiliary Effect Slot objects
    LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
    LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
    LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot = nullptr;
    LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;
    LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv = nullptr;
    LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf = nullptr;
    LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv = nullptr;
    LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti = nullptr;
    LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv = nullptr;
    LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf = nullptr;
    LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv = nullptr;

    // Helper function to load ogg files
    ALuint load_ogg(const std::string& path) {
        FILE* fp = fopen(path.c_str(), "rb");
        if (!fp) {
            throw std::runtime_error("Could not open Ogg file: " + path);
        }

        OggVorbis_File vf;
        if (ov_open_callbacks(fp, &vf, NULL, 0, OV_CALLBACKS_DEFAULT) < 0) {
            fclose(fp);
            throw std::runtime_error("ov_open_callbacks failed for: " + path);
        }

        vorbis_info* vi = ov_info(&vf, -1);
        ALenum format = (vi->channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

        int eof = 0;
        long total_size = 0;
        char pcmout[4096];
        std::vector<char> buffer_data;

        while(!eof) {
            long ret = ov_read(&vf, pcmout, sizeof(pcmout), 0, 2, 1, &eof);
            if (ret < 0) {
                ov_clear(&vf);
                throw std::runtime_error("Error in the ogg stream: " + path);
            } else if (ret > 0) {
                buffer_data.insert(buffer_data.end(), pcmout, pcmout + ret);
                total_size += ret;
            }
        }

        ALuint buffer = 0;
        alGenBuffers(1, &buffer);
        if (alGetError() != AL_NO_ERROR) {
            ov_clear(&vf);
            throw std::runtime_error("alGenBuffers failed");
        }

        alBufferData(buffer, format, buffer_data.data(), total_size, vi->rate);
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            alDeleteBuffers(1, &buffer);
            ov_clear(&vf);
            throw std::runtime_error("alBufferData failed with error: " + std::to_string(error));
        }

        ov_clear(&vf); // This also closes the file pointer
        return buffer;
    }
}
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
    : window(sf::VideoMode({800, 600}), "Stealth Action - Coordinated Assault", sf::Style::Titlebar | sf::Style::Close)
{
    logError("DEBUG_LOG: Constructor start.");
    setupConsole();
    logError("DEBUG_LOG: Console setup.");
    loadSettings();
    logError("DEBUG_LOG: Settings loaded.");

    try {
        InitOpenAL();
        audioInitialized = true;
        logError("DEBUG_LOG: Loading sounds...");
        loadSounds();
        logError("DEBUG_LOG: Sounds loaded.");
        logError("DEBUG_LOG: Generating sounds...");
        generateSounds();
        logError("DEBUG_LOG: Sounds generated.");

        // Set a default reverb preset
        setReverbPreset(EFX_REVERB_PRESET_GENERIC);
        logError("DEBUG_LOG: Default reverb preset set.");

    } catch (const std::exception& e) {
        logError("AUDIO_ERROR: Failed to initialize sounds. Game will run without audio. Error: " + std::string(e.what()));
        audioInitialized = false;
        openalBuffers.clear();
    }

    logError("DEBUG_LOG: Creating player...");
    player = std::make_unique<Player>(settings);
    logError("DEBUG_LOG: Player created.");

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

SoundEngine::~SoundEngine() {
    ShutdownOpenAL();
}

void SoundEngine::setReverbPreset(const EFXEAXREVERBPROPERTIES& preset) {
    if (!audioInitialized || !alEffecti || reverbEffect == 0) return;

    alEffectf(reverbEffect, AL_REVERB_DENSITY, preset.flDensity);
    alEffectf(reverbEffect, AL_REVERB_DIFFUSION, preset.flDiffusion);
    alEffectf(reverbEffect, AL_REVERB_GAIN, preset.flGain);
    alEffectf(reverbEffect, AL_REVERB_GAINHF, preset.flGainHF);
    alEffectf(reverbEffect, AL_REVERB_DECAY_TIME, preset.flDecayTime);
    alEffectf(reverbEffect, AL_REVERB_DECAY_HFRATIO, preset.flDecayHFRatio);
    alEffectf(reverbEffect, AL_REVERB_REFLECTIONS_GAIN, preset.flReflectionsGain);
    alEffectf(reverbEffect, AL_REVERB_REFLECTIONS_DELAY, preset.flReflectionsDelay);
    alEffectf(reverbEffect, AL_REVERB_LATE_REVERB_GAIN, preset.flLateReverbGain);
    alEffectf(reverbEffect, AL_REVERB_LATE_REVERB_DELAY, preset.flLateReverbDelay);
    alEffectf(reverbEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, preset.flAirAbsorptionGainHF);
    alEffectf(reverbEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, preset.flRoomRolloffFactor);
    alEffecti(reverbEffect, AL_REVERB_DECAY_HFLIMIT, (ALint)preset.iDecayHFLimit);

    // After setting the effect, we need to attach it to the slot again.
    alAuxiliaryEffectSloti(effectSlot, AL_EFFECTSLOT_EFFECT, reverbEffect);
}


void SoundEngine::InitOpenAL() {
    openalDevice = alcOpenDevice(nullptr);
    if (!openalDevice) {
        throw std::runtime_error("Failed to open OpenAL device");
    }

    openalContext = alcCreateContext(openalDevice, nullptr);
    if (!openalContext) {
        alcCloseDevice(openalDevice);
        throw std::runtime_error("Failed to create OpenAL context");
    }

    if (!alcMakeContextCurrent(openalContext)) {
        alcDestroyContext(openalContext);
        alcCloseDevice(openalDevice);
        throw std::runtime_error("Failed to make OpenAL context current");
    }

    // --- Загрузка функций EFX ---
    if (alcIsExtensionPresent(openalDevice, "ALC_EXT_EFX") == AL_FALSE) {
        logError("AUDIO_WARNING: EFX extension not available on this device. Reverb will be disabled.");
    } else {
        logError("DEBUG_LOG: EFX extension found. Loading function pointers...");
        #define LOAD_PROC(T, x) ((x) = (T)alGetProcAddress(#x))
        LOAD_PROC(LPALGENEFFECTS, alGenEffects);
        LOAD_PROC(LPALDELETEEFFECTS, alDeleteEffects);
        LOAD_PROC(LPALISEFFECT, alIsEffect);
        LOAD_PROC(LPALEFFECTI, alEffecti);
        LOAD_PROC(LPALEFFECTIV, alEffectiv);
        LOAD_PROC(LPALEFFECTF, alEffectf);
        LOAD_PROC(LPALEFFECTFV, alEffectfv);
        LOAD_PROC(LPALGETEFFECTI, alGetEffecti);
        LOAD_PROC(LPALGETEFFECTIV, alGetEffectiv);
        LOAD_PROC(LPALGETEFFECTF, alGetEffectf);
        LOAD_PROC(LPALGETEFFECTFV, alGetEffectfv);
        LOAD_PROC(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots);
        LOAD_PROC(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots);
        LOAD_PROC(LPALISAUXILIARYEFFECTSLOT, alIsAuxiliaryEffectSlot);
        LOAD_PROC(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti);
        LOAD_PROC(LPALAUXILIARYEFFECTSLOTIV, alAuxiliaryEffectSlotiv);
        LOAD_PROC(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf);
        LOAD_PROC(LPALAUXILIARYEFFECTSLOTFV, alAuxiliaryEffectSlotfv);
        LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTI, alGetAuxiliaryEffectSloti);
        LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTIV, alGetAuxiliaryEffectSlotiv);
        LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTF, alGetAuxiliaryEffectSlotf);
        LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTFV, alGetAuxiliaryEffectSlotfv);
        #undef LOAD_PROC

        // Создаем эффект реверберации
        alGenEffects(1, &reverbEffect);
        if (alGetError() != AL_NO_ERROR) {
            throw std::runtime_error("Failed to generate EFX effect");
        }
        alEffecti(reverbEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
        if (alGetError() != AL_NO_ERROR) {
            throw std::runtime_error("Failed to set EFX effect type");
        }

        // Создаем слот для эффекта
        alGenAuxiliaryEffectSlots(1, &effectSlot);
        if (alGetError() != AL_NO_ERROR) {
            throw std::runtime_error("Failed to generate EFX slot");
        }
        logError("DEBUG_LOG: EFX reverb effect created.");
    }

    // Создаем пул источников звука
    soundSources.resize(SOUND_POOL_SIZE);
    alGenSources(SOUND_POOL_SIZE, soundSources.data());
    if (alGetError() != AL_NO_ERROR) {
        throw std::runtime_error("Failed to generate OpenAL sources");
    }

    // Создаем выделенные источники для зацикленных звуков
    alGenSources(1, &lowHealthSoundSource);
    alGenSources(1, &shadowSoundSource);

    logError("DEBUG_LOG: OpenAL initialized successfully.");
}

void SoundEngine::ShutdownOpenAL() {
    if (openalContext) {
        // Stop all sources
        alSourceStopv(soundSources.size(), soundSources.data());
        alSourceStop(lowHealthSoundSource);
        alSourceStop(shadowSoundSource);

        // Delete sources and buffers
        alDeleteSources(soundSources.size(), soundSources.data());
        alDeleteSources(1, &lowHealthSoundSource);
        alDeleteSources(1, &shadowSoundSource);
        std::vector<ALuint> buffersToDelete;
        for(auto const& [name, buffer] : openalBuffers) {
            buffersToDelete.push_back(buffer);
        }
        alDeleteBuffers(buffersToDelete.size(), buffersToDelete.data());
        openalBuffers.clear();

        if (alDeleteAuxiliaryEffectSlots && effectSlot != 0) {
            alDeleteAuxiliaryEffectSlots(1, &effectSlot);
        }
        if (alDeleteEffects && reverbEffect != 0) {
            alDeleteEffects(1, &reverbEffect);
        }

        alcMakeContextCurrent(nullptr);
        alcDestroyContext(openalContext);
    }
    if (openalDevice) {
        alcCloseDevice(openalDevice);
    }
    logError("DEBUG_LOG: OpenAL shut down.");
}

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

        try {
            openalBuffers[name] = load_ogg(path);
        } catch (const std::runtime_error& e) {
            try {
                std::string fallbackPath = "sounds/" + name + ".ogg";
                openalBuffers[name] = load_ogg(fallbackPath);
            } catch (const std::runtime_error& e2) {
                throw std::runtime_error("Failed to load sound " + name + ": " + e.what());
            }
        }
    }
}

void SoundEngine::generateSounds() {
    std::vector<std::int16_t> samples;

    auto create_buffer = [&](const std::string& name, const std::vector<std::int16_t>& sample_data, int channels, int sample_rate) {
        if (openalBuffers.find(name) == openalBuffers.end()) {
            ALuint buffer;
            alGenBuffers(1, &buffer);
            ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
            alBufferData(buffer, format, sample_data.data(), sample_data.size() * sizeof(std::int16_t), sample_rate);
            if (alGetError() != AL_NO_ERROR) {
                throw std::runtime_error("Failed to create OpenAL buffer for generated sound: " + name);
            }
            openalBuffers[name] = buffer;
        }
    };

    samples.assign(44100 / 10, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(15000.0f * sin(2*3.14159f*600.0f*t) * exp(-t*30.0f)); }
    create_buffer("DetectionTick", samples, 1, 44100);
    samples.assign(44100 / 2, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(32000.0f * sin(2*3.14159f*440.0f*t) * (1.0f-t*2.0f)); }
    create_buffer("Spotted", samples, 1, 44100);
    samples.assign(44100, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; float pulse = (sin(2*3.14159f*2.0f*t)+1.0f)/2.0f; samples[i] = static_cast<std::int16_t>(20000.0f * sin(2*3.14159f*300.0f*t) * pulse); }
    create_buffer("LowHealth", samples, 1, 44100);
    samples.assign(44100/5, 0); for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(32000.0f * sin(2*3.14159f*900.0f*t) * exp(-t*20.0f)); }
    create_buffer("sonar", samples, 1, 44100);
    samples.assign(44100/30, 0);for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(18000.0f * sin(2*3.14159f*1200.0f*t) * exp(-t*80.0f)); }
    create_buffer("sonar_echo", samples, 1, 44100);
    samples.assign(44100/20, 0); for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(25000.0f*sin(2*3.14159f*880.0f*t)*exp(-t*50.0f)); }
    create_buffer("HealthIndicator", samples, 1, 44100);
    samples.assign(44100/15, 0); for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(20000.0f*sin(2*3.14159f*600.0f*t)*exp(-t*40.0f)); }
    create_buffer("MenuSelect", samples, 1, 44100);
    samples.assign(44100/10, 0); for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; samples[i] = static_cast<std::int16_t>(22000.0f*sin(2*3.14159f*440.0f*t)*exp(-t*30.0f)); }
    create_buffer("MenuConfirm", samples, 1, 44100);
    samples.assign(44100/8, 0); for (size_t i=0; i < samples.size(); ++i) { float t = static_cast<float>(i)/44100.0f; float freq = 800.0f - sin(t * 3.14159f * 4.0f) * 400.0f; samples[i] = static_cast<std::int16_t>(28000.0f * sin(2*3.14159f*freq*t) * exp(-t*20.0f)); }
    create_buffer("Stun", samples, 1, 44100);
    samples.assign(44100 / 4, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i) / 44100.0f; float noise = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f; samples[i] = static_cast<std::int16_t>(25000.0f * (sin(2 * 3.14159f * 1200.0f * t) + 0.5f * noise) * exp(-t * 15.0f)); }
    create_buffer("Taser_Fire", samples, 1, 44100);
    samples.assign(44100 / 7, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i) / 44100.0f; float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f; float envelope = exp(-t * 40.0f); samples[i] = static_cast<std::int16_t>(30000.0f * noise * envelope); }
    create_buffer("Knife_Swish", samples, 1, 44100);
    samples.assign(44100 / 4, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i) / 44100.0f; float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f; float envelope = exp(-t * 20.0f); samples[i] = static_cast<std::int16_t>(25000.0f * noise * envelope); }
    create_buffer("Machete_Swish", samples, 1, 44100);
    samples.assign(44100 / 5, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i) / 44100.0f; float freq = 600.0f - t * 3000.0f; samples[i] = static_cast<std::int16_t>(22000.0f * sin(2 * 3.14159f * freq * t) * exp(-t * 20.0f)); }
    create_buffer("Bat_Swish", samples, 1, 44100);
    samples.assign(44100 / 6, 0); for (size_t i = 0; i < samples.size(); ++i) { float t = static_cast<float>(i) / 44100.0f; float freq = 800.0f - t * 2500.0f; samples[i] = static_cast<std::int16_t>(20000.0f * (sin(2 * 3.14159f * freq * t) + 0.2f * sin(2 * 3.14159f * freq * 2.5f * t)) * exp(-t * 30.0f)); }
    create_buffer("Blunt_Metal_Swish", samples, 1, 44100);
    samples.assign(44100, 0); for (size_t i = 0; i < samples.size(); ++i) { samples[i] = static_cast<std::int16_t>((static_cast<float>(rand()) / RAND_MAX - 0.5f) * 4000.0f); }
    create_buffer("Shadow_Ambience", samples, 1, 44100);
}

void SoundEngine::run() {
    logError("DEBUG_LOG: run() started.");
    window.setVerticalSyncEnabled(true);
    resetGame();
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
                float noiseRadius = 0.0f;
                if (player->isRunning) {
                    noiseRadius = 25.0f;
                } else if (player->isCrouching) {
                    noiseRadius = 3.0f;
                } else {
                    noiseRadius = 10.0f;
                }
                StealthSystem::processPlayerNoise(*player, enemies, noiseRadius);
                stepClock.restart();
            }
        } else if (gameState == GameState::PlayerDying) {
            static bool timerStarted = false;
            static sf::Clock deathTimer;
            if (!timerStarted) {
                deathTimer.restart();
                timerStarted = true;
            }

            if (deathTimer.getElapsedTime().asSeconds() > 2.5f) { // Wait for sound to play
                gameState = GameState::GameOver;
                alSourceStop(lowHealthSoundSource);
                alSourceStop(shadowSoundSource);
                timerStarted = false;
            }
        }
        render();
        sf::sleep(sf::milliseconds(10));
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
                    case sf::Keyboard::Key::F: handlePlayerTakedown(); break;
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

    // Dynamic reverb logic
    static bool in_cave = false; // simple state
    bool currently_in_cave = (player->position.x > 50.f); // Define a "cave" area
    if (currently_in_cave && !in_cave) {
        setReverbPreset(EFX_REVERB_PRESET_CAVE);
        logError("DEBUG_LOG: Switched to CAVE reverb preset.");
        in_cave = true;
    } else if (!currently_in_cave && in_cave) {
        setReverbPreset(EFX_REVERB_PRESET_GENERIC);
        logError("DEBUG_LOG: Switched to GENERIC reverb preset.");
        in_cave = false;
    }

    updateLowHealthSound();
    updateShadowSound();
    if (gameMode == GameMode::CLASSIC_ACTION) { updateProximitySonar(); }
    if (!player->isAlive && gameState == GameState::Playing) {
        gameState = GameState::PlayerDying;
        playSound("player_death", {0,0,0}, 100.f, true);
    }
}

void SoundEngine::updateShadowSound() {
    if (!audioInitialized) return;

    bool isInShadow = StealthSystem::isInShadow(player->position, shadowZones);
    ALint sourceState;
    alGetSourcei(shadowSoundSource, AL_SOURCE_STATE, &sourceState);

    if (isInShadow && sourceState != AL_PLAYING) {
        alSourcei(shadowSoundSource, AL_BUFFER, openalBuffers.at("Shadow_Ambience"));
        alSourcef(shadowSoundSource, AL_GAIN, 30.0f / 100.f);
        alSourcei(shadowSoundSource, AL_SOURCE_RELATIVE, AL_TRUE);
        alSourcei(shadowSoundSource, AL_LOOPING, AL_TRUE);
        alSourcePlay(shadowSoundSource);
    } else if (!isInShadow && sourceState == AL_PLAYING) {
        alSourceStop(shadowSoundSource);
        alSourcei(shadowSoundSource, AL_BUFFER, 0);
    }
}

void SoundEngine::generateLevel() {
    walls.clear();
    shadowZones.clear();
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

void SoundEngine::resetGame() {
    logError("DEBUG_LOG: resetGame() started.");
    gameState = GameState::Playing;
    player->reset(settings);
    generateLevel();
    alListener3f(AL_DIRECTION, 0.f, 0.f, -1.f);
    enemies.clear();
    for (int i=0; i < INITIAL_NPC_COUNT; ++i) {
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
}

void SoundEngine::handleEnemyActions(float deltaTime) {
    for (auto& enemy : enemies) {
        if (enemy->isAlive) {
            if (enemy->state != AIState::COMBAT) {
                StealthSystem::updateDetection(*enemy, *player, walls, shadowZones, settings, deltaTime);
                if (enemy->detectionLevel >= 100.f) {
                    onEnemySpottedPlayer(enemy.get(), true);
                }
            }
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
                           onEnemySpottedPlayer(enemy.get(), true);
                           break;
                        }
                    }
                }
            }
            enemy->update(deltaTime, *player, *this, settings, gameMode, walls, enemies);
        }
    }
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
    if (!player->isAlive || player->isStunned) return;
    const auto& weapon = weapons.at(player->currentWeapon);
    if (player->lastAttackClock.getElapsedTime().asSeconds() < weapon.cooldown) return;
    player->lastAttackClock.restart();
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
                bestTarget->takeDamage(0, *this, player.get(), true);
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
    if (player->lastAttackClock.getElapsedTime().asSeconds() < 0.5f) return;
    player->lastAttackClock.restart();
    Enemy* target = nullptr;
    float minDistance = 1.5f;
    for (auto& enemy : enemies) {
        if (!enemy->isAlive || enemy->state == AIState::COMBAT) continue;
        float dist = std::hypot(player->position.x - enemy->position.x, player->position.z - enemy->position.z);
        if (dist < minDistance) {
            minDistance = dist;
            target = enemy.get();
        }
    }
    if (target) {
        playSound("takedown", target->position);
        target->takeDamage(1000, *this, player.get());
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
        float volume = 20.0f + distanceFactor * 80.0f;
        float pitch = 1.0f + distanceFactor * 1.5f;
        playSound("sonar", {0,0,0}, volume, true, pitch);
        proximitySonarClock.restart();
    }
}

void SoundEngine::updateLowHealthSound() {
    if (!audioInitialized) return;
    bool isHealthLow = player->isAlive && (player->health <= settings.lowHealthThreshold);
    ALint sourceState;
    alGetSourcei(lowHealthSoundSource, AL_SOURCE_STATE, &sourceState);

    if (isHealthLow && sourceState != AL_PLAYING) {
        alSourcei(lowHealthSoundSource, AL_BUFFER, openalBuffers.at("LowHealth"));
        alSourcef(lowHealthSoundSource, AL_GAIN, 80.0f / 100.f);
        alSourcei(lowHealthSoundSource, AL_SOURCE_RELATIVE, AL_TRUE);
        alSourcei(lowHealthSoundSource, AL_LOOPING, AL_TRUE);
        alSourcePlay(lowHealthSoundSource);
    } else if (!isHealthLow && sourceState == AL_PLAYING) {
        alSourceStop(lowHealthSoundSource);
        alSourcei(lowHealthSoundSource, AL_BUFFER, 0);
    }
}

void SoundEngine::onEnemySpottedPlayer(Enemy* spottedBy, bool forceCombat) {
    if (spottedBy->state == AIState::COMBAT) return;

    spottedBy->state = AIState::COMBAT;
    spottedBy->detectionLevel = 0;

    if (getInt(1, 100) <= 70) {
        playSound("battle_cry", spottedBy->position, 100.f, false, getFloat(0.9f, 1.1f));
    }

    // Stop detection sound since combat has started
    // This logic needs to be adapted for OpenAL sources
}

void SoundEngine::onEnemyDied(Enemy* deadEnemy) {
    playSound("death", deadEnemy->position);
}

void SoundEngine::playSound(const std::string& name, sf::Vector3f position, float volume, bool isListenerRelative, float pitch) {
    if (!audioInitialized) return;
    auto it = openalBuffers.find(name);
    if (it == openalBuffers.end()) {
        logError("Attempted to play sound that is not loaded: " + name);
        return;
    }

    ALuint source = soundSources[currentSourceIndex];
    alSourceStop(source);
    alSourcei(source, AL_BUFFER, 0);

    alSourcei(source, AL_BUFFER, it->second);
    alSourcef(source, AL_PITCH, (pitch == -1.f) ? getFloat(0.95f, 1.05f) : pitch);
    alSourcef(source, AL_GAIN, volume / 100.f);
    alSourcei(source, AL_SOURCE_RELATIVE, isListenerRelative ? AL_TRUE : AL_FALSE);

    if (isListenerRelative) {
        alSource3f(source, AL_POSITION, 0, 0, 0);
        alSource3i(source, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
    } else {
        alSource3f(source, AL_POSITION, position.x, position.y, position.z);
        alSourcef(source, AL_REFERENCE_DISTANCE, 4.0f);
        alSourcef(source, AL_ROLLOFF_FACTOR, 1.5f);
        if (effectSlot != 0) {
            alSource3i(source, AL_AUXILIARY_SEND_FILTER, effectSlot, 0, AL_FILTER_NULL);
        }
    }

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        logError("OpenAL error before playing sound " + name + ": " + std::to_string(error));
        return;
    }

    alSourcePlay(source);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        logError("OpenAL error after playing sound " + name + ": " + std::to_string(error));
    }

    currentSourceIndex = (currentSourceIndex + 1) % SOUND_POOL_SIZE;
}

void SoundEngine::render() {
    window.clear(sf::Color(10, 10, 20));
    sf::View view;
    view.setCenter({player->position.x, player->position.z});
    view.setSize({80.f, 60.f});
    window.setView(view);

    sf::RectangleShape wallShape;
    wallShape.setFillColor(sf::Color(100, 120, 140));
    for (const auto& wallRect : walls) {
        wallShape.setPosition({wallRect.position.x, wallRect.position.y});
        wallShape.setSize(wallRect.size);
        window.draw(wallShape);
    }

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