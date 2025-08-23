cmake_minimum_required(VERSION 3.16)
project(StealthActionGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define SFML_STATIC before finding the package.
# This is a hint to FindSFML.cmake to look for static libraries.
set(SFML_STATIC_LIBRARIES TRUE)

find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Audio Window System)

set(PROJECT_SOURCES
    main.cpp
    SoundEngine.cpp
    entities.cpp
)

add_executable(StealthActionGame ${PROJECT_SOURCES})

# Add the SFML_STATIC definition for the preprocessor.
# This is crucial for the SFML headers to be configured for static linking.
target_compile_definitions(StealthActionGame PRIVATE SFML_STATIC)

# Link the SFML targets. Modern CMake with vcpkg should handle transitive dependencies.
target_link_libraries(StealthActionGame PRIVATE
    SFML::Graphics
    SFML::Audio
    SFML::Window
    SFML::System
)

# For MinGW static builds, we sometimes need to link system libraries manually,
# even when using a package manager like vcpkg. This is a common source of linker errors.
if(CMAKE_COMPILER_IS_GNUCXX AND WIN32)
    target_link_libraries(StealthActionGame PRIVATE
        opengl32
        winmm
        gdi32
        ws2_32
    )
endif()

# The executable will be created in the root of the build directory.
# This matches the original behavior.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
