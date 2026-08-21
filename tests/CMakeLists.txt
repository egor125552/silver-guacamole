include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(AllTests
    test_hardening.cpp
    test_utils.cpp
)

target_link_libraries(AllTests PRIVATE gtest_main GameLogic)
gtest_discover_tests(AllTests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
