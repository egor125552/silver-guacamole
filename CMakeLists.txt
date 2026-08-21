cmake_minimum_required(VERSION 3.16)
project(StealthActionGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Window System)
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENAL REQUIRED IMPORTED_TARGET openal)
pkg_check_modules(VORBISFILE REQUIRED IMPORTED_TARGET vorbisfile)

include(cmake/PrepareSoundEngine.cmake)

set(GAME_LOGIC_SOURCES
    ${GENERATED_SOUNDENGINE}
    SoundEngine.h
    Character.cpp
    Character.h
    Player.cpp
    Player.h
    Enemy.cpp
    Enemy.h
    Stealth.cpp
    Stealth.h
    common.h
    utils.h
    ai_definitions.h
)

add_library(GameLogic STATIC ${GAME_LOGIC_SOURCES})
target_include_directories(GameLogic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GameLogic
    PUBLIC
        SFML::Graphics
        SFML::Window
        SFML::System
        PkgConfig::OPENAL
    PRIVATE
        PkgConfig::VORBISFILE
)

add_executable(StealthActionGame main.cpp)
target_link_libraries(StealthActionGame PRIVATE GameLogic)

if(WIN32)
    target_link_libraries(StealthActionGame PRIVATE avrt)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
