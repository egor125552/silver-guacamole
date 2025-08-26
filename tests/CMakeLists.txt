# CMakeLists.txt для тестов

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.1
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Находим все файлы с тестами (*.cpp) в этой директории
file(GLOB TEST_SOURCES "*.cpp")

# Создаем один исполняемый файл "AllTests", который включает все найденные тесты
add_executable(
    AllTests
    ${TEST_SOURCES}
)

# Линкуем исполняемый файл с gtest и нашей основной библиотекой GameLogic
target_link_libraries(
    AllTests
    PRIVATE
    gtest_main
    GameLogic
)

# Автоматически находим все тесты
# Устанавливаем рабочую директорию, чтобы тесты могли найти файлы ресурсов (config.ini, sounds/)
# CMAKE_SOURCE_DIR - это корень проекта
gtest_discover_tests(AllTests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
