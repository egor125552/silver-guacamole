cmake_minimum_required(VERSION 3.16)
project(StealthActionGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_STATIC_LIBRARIES ON)
find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Window System)
find_package(OpenAL REQUIRED)
find_package(fmt REQUIRED)

# --- Поиск Ogg/Vorbis через pkg-config ---
# В некоторых средах стандартный find_package не может найти Ogg и Vorbis.
# Использование pkg-config - более надежный метод для Linux.
find_package(PkgConfig REQUIRED)
pkg_check_modules(OGG REQUIRED IMPORTED_TARGET ogg)
pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis vorbisfile)


# fmt, как и другие зависимости, будет найден через find_package

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
        $<$<CONFIG:Debug>:fmtd>$<$<NOT:$<CONFIG:Debug>>:fmt>
        PkgConfig::OGG
        PkgConfig::VORBIS
)

# Публичные зависимости (заголовки)
# Делаем так, чтобы потребители библиотеки (игра и тесты) могли найти заголовки
target_include_directories(GameLogic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(GameLogic INTERFACE SFML_STATIC)


# --- Основной исполняемый файл ---
# Теперь он просто использует библиотеку GameLogic
add_executable(StealthActionGame main.cpp)
target_link_libraries(StealthActionGame PRIVATE GameLogic)

# Для MinGW/Windows требуется явно линковать некоторые зависимости,
# которые не подтягиваются автоматически через статическую библиотеку GameLogic.
if(WIN32)
    # Используем прямые имена ('fmt', 'avrt') вместо импортированных таргетов,
    # так как это более надежно в данной среде.
    # Для fmt используем генераторное выражение, чтобы выбрать debug/release версию.
    target_link_libraries(StealthActionGame PRIVATE $<$<CONFIG:Debug>:fmtd>$<$<NOT:$<CONFIG:Debug>>:fmt> avrt)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# --- Windows-specific post-build step ---
# This command copies the OpenAL DLL to the directory where the executable is created.
# This is crucial for running the game on Windows, as the system needs to find this DLL at runtime.
if(WIN32)
    # The path assumes a standard vcpkg setup as described in agents.MD.
    # The actual DLL is usually soft_oal.dll for OpenAL-Soft.
    set(VCPKG_BIN_DIR "")
    if(DEFINED ENV{VCPKG_ROOT})
        set(VCPKG_BIN_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows/bin")
    elseif(DEFINED CMAKE_TOOLCHAIN_FILE)
        # Fallback for when VCPKG_ROOT is not an env var, but cmake toolchain is used
        get_filename_component(VCPKG_TOOLCHAIN_PATH "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
        set(VCPKG_BIN_DIR "${VCPKG_TOOLCHAIN_PATH}/../installed/x64-windows/bin")
    endif()

    if(EXISTS "${VCPKG_BIN_DIR}")
        find_file(OPENAL_DLL
            NAMES soft_oal.dll OpenAL32.dll
            HINTS ${VCPKG_BIN_DIR}
            NO_DEFAULT_PATH
        )
    endif()

    if(OPENAL_DLL)
        message(STATUS "Found OpenAL DLL: ${OPENAL_DLL}, will copy to output directory.")
        add_custom_command(TARGET StealthActionGame POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENAL_DLL}"
            "$<TARGET_FILE_DIR:StealthActionGame>"
            COMMENT "Copying OpenAL DLL to output directory..."
        )
    else()
        message(WARNING "OpenAL DLL not found. The application may fail to run on Windows. Searched in: ${VCPKG_BIN_DIR}")
    endif()
endif()


# --- Тестирование ---
enable_testing()
add_subdirectory(tests)