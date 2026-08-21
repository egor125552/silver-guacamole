# Build a generated copy of SoundEngine.cpp with platform-specific input hardening
# and small glue fixes. Every replacement is required, so source drift fails loudly
# during configure instead of silently producing a broken app.

set(SOUNDENGINE_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/SoundEngine.cpp")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_SOUNDENGINE "${GENERATED_DIR}/SoundEngine.cpp")

file(READ "${SOUNDENGINE_SOURCE_PATH}" SOUNDENGINE_SOURCE)

function(replace_required old_text new_text label)
    string(FIND "${SOUNDENGINE_SOURCE}" "${old_text}" match_pos)
    if(match_pos EQUAL -1)
        message(FATAL_ERROR "Could not apply SoundEngine patch: ${label}")
    endif()
    string(REPLACE "${old_text}" "${new_text}" SOUNDENGINE_SOURCE "${SOUNDENGINE_SOURCE}")
    set(SOUNDENGINE_SOURCE "${SOUNDENGINE_SOURCE}" PARENT_SCOPE)
endfunction()

# Native macOS keyboard state is a fallback for keys intercepted by VoiceOver or SFML.
replace_required(
    "#include <cmath>\n\n#ifdef _WIN32"
    "#include <cmath>\n\n#ifdef __APPLE__\n#include <ApplicationServices/ApplicationServices.h>\n#endif\n\n#ifdef _WIN32"
    "ApplicationServices include"
)

replace_required(
    "namespace {\nLPALGENEFFECTS alGenEffects = nullptr;"
    "namespace {\nbool platformKeyDown(unsigned short macKeyCode) {\n#ifdef __APPLE__\n    return CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, static_cast<CGKeyCode>(macKeyCode));\n#else\n    (void)macKeyCode;\n    return false;\n#endif\n}\n\nLPALGENEFFECTS alGenEffects = nullptr;"
    "native key helper"
)

replace_required(
    "    window.setVerticalSyncEnabled(true);\n    resetGame();"
    "    window.setVerticalSyncEnabled(true);\n    window.requestFocus();\n    resetGame();"
    "window focus"
)

replace_required(
    "    player->isRunning = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::RShift);"
    "    player->isRunning = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::RShift) || platformKeyDown(56) || platformKeyDown(60);"
    "Shift polling"
)

replace_required(
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Up)) moveDir.z -= 1.f;"
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || platformKeyDown(126) || platformKeyDown(13)) moveDir.z -= 1.f;"
    "Up polling"
)
replace_required(
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Down)) moveDir.z += 1.f;"
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || platformKeyDown(125) || platformKeyDown(1)) moveDir.z += 1.f;"
    "Down polling"
)
replace_required(
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Left)) moveDir.x -= 1.f;"
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || platformKeyDown(123) || platformKeyDown(0)) moveDir.x -= 1.f;"
    "Left polling"
)
replace_required(
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Right)) moveDir.x += 1.f;"
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || platformKeyDown(124) || platformKeyDown(2)) moveDir.x += 1.f;"
    "Right polling"
)

replace_required(
    "    if (current.isAutomatic && sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space)) handlePlayerAttack();"
    "    if (current.isAutomatic && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space) || platformKeyDown(49))) handlePlayerAttack();"
    "held Space polling"
)

replace_required(
    "                StealthSystem::processPlayerNoise(*player, enemies, noise);"
    "                StealthSystem::processPlayerNoise(*player, enemies, walls, noise);"
    "wall-aware player noise"
)

# The two historical scene recordings were loaded by the old game but never used.
# Trigger them occasionally when suitable non-combat NPCs are actually near each other.
replace_required(
    "void SoundEngine::handleEnemyActions(float deltaTime) {\n    if (gameClock.getElapsedTime().asSeconds() < settings.gracePeriod) return;"
    "void SoundEngine::handleEnemyActions(float deltaTime) {\n    if (gameClock.getElapsedTime().asSeconds() < settings.gracePeriod) return;\n\n    static sf::Clock ambientSceneClock;\n    if (ambientSceneClock.getElapsedTime().asSeconds() > 38.f) {\n        Enemy* quietGuard = nullptr;\n        Enemy* quietPrisoner = nullptr;\n        Enemy* secondPrisoner = nullptr;\n        for (auto& enemy : enemies) {\n            if (!enemy->isAlive || enemy->state == AIState::COMBAT || enemy->state == AIState::ALERT) continue;\n            if (enemy->type == NPCType::GUARD && !quietGuard) quietGuard = enemy.get();\n            if (enemy->type == NPCType::REGULAR) {\n                if (!quietPrisoner) quietPrisoner = enemy.get();\n                else if (!secondPrisoner) secondPrisoner = enemy.get();\n            }\n        }\n\n        bool playedScene = false;\n        if (quietGuard && quietPrisoner && planarDistance(quietGuard->position, quietPrisoner->position) < 14.f) {\n            const sf::Vector3f scenePos = (quietGuard->position + quietPrisoner->position) * 0.5f;\n            playSound(\"guard_vs_lukashenko_main\", scenePos, 72.f);\n            playedScene = true;\n        } else if (quietPrisoner && secondPrisoner && planarDistance(quietPrisoner->position, secondPrisoner->position) < 12.f) {\n            const sf::Vector3f scenePos = (quietPrisoner->position + secondPrisoner->position) * 0.5f;\n            playSound(\"prisoners_arguing_main\", scenePos, 68.f);\n            playedScene = true;\n        }\n\n        if (playedScene || ambientSceneClock.getElapsedTime().asSeconds() > 65.f) ambientSceneClock.restart();\n    }"
    "historical ambient scenes"
)

file(MAKE_DIRECTORY "${GENERATED_DIR}")
file(WRITE "${GENERATED_SOUNDENGINE}" "${SOUNDENGINE_SOURCE}")
