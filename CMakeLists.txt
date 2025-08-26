cmake_minimum_required(VERSION 3.16)
project(StealthActionGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_STATIC_LIBRARIES ON)
find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Window System)
find_package(OpenAL REQUIRED)
find_package(fmt REQUIRED)

set(PROJECT_SOURCES
    main.cpp
    SoundEngine.cpp
    Character.cpp
    Player.cpp
    Enemy.cpp
    Stealth.cpp
)

add_executable(StealthActionGame ${PROJECT_SOURCES})

target_compile_definitions(StealthActionGame PRIVATE SFML_STATIC)

target_link_libraries(StealthActionGame PRIVATE SFML::Graphics SFML::Window SFML::System OpenAL::OpenAL fmt::fmt ogg vorbis vorbisfile)

# Принудительно указываем путь к заголовочным файлам SFML 3.0 (на всякий случай)
target_include_directories(StealthActionGame PRIVATE ${SFML_INCLUDE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# --- Тестирование ---
enable_testing()
add_subdirectory(tests)