cmake_minimum_required(VERSION 3.16)
project(StealthActionGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_STATIC_LIBRARIES ON)
find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Window System)
find_package(OpenAL REQUIRED)
find_package(fmt REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OGG REQUIRED IMPORTED_TARGET ogg)
pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis vorbisfile)

set(GAME_LOGIC_SOURCES
    ../SoundEngine.cpp
    ../SoundEngine.h
    ../Character.cpp
    ../Character.h
    ../Player.cpp
    ../Player.h
    ../Enemy.cpp
    ../Enemy.h
    ../Stealth.cpp
    ../Stealth.h
    ../common.h
    ../utils.h
    ../ai_definitions.h
)

add_library(GameLogic STATIC ${GAME_LOGIC_SOURCES})

target_link_libraries(GameLogic
    PUBLIC
        SFML::Graphics SFML::Window SFML::System
    PRIVATE
        OpenAL::OpenAL
        fmt::fmt
        PkgConfig::OGG
        PkgConfig::VORBIS
)

target_include_directories(GameLogic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(GameLogic INTERFACE SFML_STATIC)

add_executable(StealthActionGame ../main.cpp)
target_link_libraries(StealthActionGame PRIVATE GameLogic)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

enable_testing()
add_subdirectory(../tests)
