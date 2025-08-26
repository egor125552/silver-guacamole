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

# На данный момент здесь больше ничего нет.
# Исполняемые файлы тестов (add_executable) и их связь с Google Test (target_link_libraries)
# будут добавлены на следующем шаге, когда мы будем писать сами тесты.
