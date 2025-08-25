cmake_minimum_required(VERSION 3.16)
project(MinimalSFMLTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_STATIC_LIBRARIES ON)
find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Window System)

add_executable(minimal_test minimal_test.cpp)

target_compile_definitions(minimal_test PRIVATE SFML_STATIC)

target_link_libraries(minimal_test PRIVATE SFML::Graphics SFML::Window SFML::System)
