cmake_minimum_required(VERSION 3.16)
project(StealthActionGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_STATIC_LIBRARIES ON)
find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Audio Window System)

set(PROJECT_SOURCES
    main.cpp
    SoundEngine.cpp
    entities.cpp
)

add_executable(StealthActionGame ${PROJECT_SOURCES})

target_compile_definitions(StealthActionGame PRIVATE SFML_STATIC)

target_link_libraries(StealthActionGame PRIVATE SFML::Graphics SFML::Audio SFML::Window SFML::System)

# Принудительно указываем путь к заголовочным файлам SFML 3.0 (на всякий случай)
target_include_directories(StealthActionGame PRIVATE ${SFML_INCLUDE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})