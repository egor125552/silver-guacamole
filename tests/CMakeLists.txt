# CMakeLists.txt for tests

# Включаем модуль для загрузки внешнего контента
include(FetchContent)

# Объявляем Google Test
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.1
)

# Делаем Google Test доступным для использования (скачивает и настраивает)
FetchContent_MakeAvailable(googletest)

# Включаем утилиты для тестирования Google Test
include(GoogleTest)

# Создаем исполняемый файл для теста реверберации
add_executable(
    test_reverb
    test_reverb.cpp
    ../SoundEngine.cpp
    ../Player.cpp
    ../Enemy.cpp
    ../Character.cpp
    ../Stealth.cpp
)

# Линкуем тест с GTest и другими необходимыми библиотеками
target_link_libraries(
    test_reverb
    PRIVATE
    gtest_main
    gmock
    SFML::Graphics
    SFML::Window
    SFML::System
    OpenAL::OpenAL
    fmt::fmt
    ogg
    vorbis
    vorbisfile
)

# Добавляем тест в систему CTest
gtest_discover_tests(test_reverb)
