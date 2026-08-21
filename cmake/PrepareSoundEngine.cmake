# Build a generated copy of SoundEngine.cpp with robust keyboard polling.
# This keeps the large historical source file untouched while making the macOS build
# accept both localized key codes and physical scancodes.

set(SOUNDENGINE_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/SoundEngine.cpp")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_SOUNDENGINE "${GENERATED_DIR}/SoundEngine.cpp")

file(READ "${SOUNDENGINE_SOURCE_PATH}" SOUNDENGINE_SOURCE)

function(replace_required old_text new_text label)
    string(FIND "${SOUNDENGINE_SOURCE}" "${old_text}" match_pos)
    if(match_pos EQUAL -1)
        message(FATAL_ERROR "Could not apply SoundEngine input patch: ${label}")
    endif()
    string(REPLACE "${old_text}" "${new_text}" SOUNDENGINE_SOURCE "${SOUNDENGINE_SOURCE}")
    set(SOUNDENGINE_SOURCE "${SOUNDENGINE_SOURCE}" PARENT_SCOPE)
endfunction()

replace_required(
    "    window.setVerticalSyncEnabled(true);\n    resetGame();"
    "    window.setVerticalSyncEnabled(true);\n    window.requestFocus();\n    resetGame();"
    "window focus"
)

replace_required(
    "    player->isRunning = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::RShift);"
    "    player->isRunning = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::RShift);"
    "Shift polling"
)

replace_required(
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Up)) moveDir.z -= 1.f;"
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) moveDir.z -= 1.f;"
    "Up polling"
)
replace_required(
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Down)) moveDir.z += 1.f;"
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) moveDir.z += 1.f;"
    "Down polling"
)
replace_required(
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Left)) moveDir.x -= 1.f;"
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) moveDir.x -= 1.f;"
    "Left polling"
)
replace_required(
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Right)) moveDir.x += 1.f;"
    "    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) moveDir.x += 1.f;"
    "Right polling"
)

replace_required(
    "    if (current.isAutomatic && sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space)) handlePlayerAttack();"
    "    if (current.isAutomatic && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space))) handlePlayerAttack();"
    "held Space polling"
)

file(MAKE_DIRECTORY "${GENERATED_DIR}")
file(WRITE "${GENERATED_SOUNDENGINE}" "${SOUNDENGINE_SOURCE}")
