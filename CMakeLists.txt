cmake_minimum_required(VERSION 3.10)

project(StealthGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable
add_executable(StealthGame src/main.cpp src/Player.cpp src/Enemy.cpp src/Fists.cpp src/Pistol.cpp src/Shocker.cpp src/Knife.cpp src/Machete.cpp src/SoundEngine.cpp)

# Tell SFML to find the static libraries
set(SFML_STATIC_LIBRARIES ON)

# Find SFML with capitalized component names
find_package(SFML 3.0.1 COMPONENTS System Window Graphics Audio REQUIRED)

# Link SFML using the namespace targets
if(SFML_FOUND)
    target_link_libraries(StealthGame PRIVATE SFML::Graphics SFML::Audio)
endif()
