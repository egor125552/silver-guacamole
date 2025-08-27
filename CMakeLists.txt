cmake_minimum_required(VERSION 3.16)
project(StealthActionGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_STATIC_LIBRARIES ON)
find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Window System)
find_package(OpenAL REQUIRED)

# Используем FetchContent для управления зависимостью fmt
include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 9.1.0 # Используем свежую стабильную версию
)
FetchContent_MakeAvailable(fmt)

# --- Библиотека игровой логики ---
# Все основные файлы игры компилируются в статическую библиотеку.
# Это позволяет переиспользовать код между основным приложением и тестами.
set(GAME_LOGIC_SOURCES
    SoundEngine.cpp
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

# Зависимости библиотеки
target_link_libraries(GameLogic
    # PUBLIC: Заголовки SFML используются в наших заголовках (SoundEngine.h),
    # поэтому должны быть видны всем, кто использует GameLogic.
    PUBLIC
        SFML::Graphics
        SFML::Window
        SFML::System
    # PRIVATE: Эти библиотеки используются только внутри .cpp файлов GameLogic.
    PRIVATE
        OpenAL::OpenAL
        fmt::fmt
        ogg
        vorbis
        vorbisfile
)

# Публичные зависимости (заголовки)
# Делаем так, чтобы потребители библиотеки (игра и тесты) могли найти заголовки
target_include_directories(GameLogic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(GameLogic INTERFACE SFML_STATIC)


# --- Основной исполняемый файл ---
# Теперь он просто использует библиотеку GameLogic
add_executable(StealthActionGame main.cpp)
target_link_libraries(StealthActionGame PRIVATE GameLogic)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# --- Тестирование ---
enable_testing()
add_subdirectory(tests)