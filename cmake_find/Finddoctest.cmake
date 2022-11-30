include(FetchContent)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
)

FetchContent_MakeAvailable(doctest)
list(APPEND CMAKE_MODULE_PATH "${doctest_SOURCE_DIR}/scripts/cmake/")
