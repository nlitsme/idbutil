list(APPEND CMAKE_MODULE_PATH "/usr/lib/cmake/doctest/")
if (TARGET doctest)
    return()
endif()
if (TARGET doctest::doctest)
    return()
endif()
file(GLOB DOCTEST_DOCTEST_DIRS /usr/include /usr/local/include /usr/local/opt/doctest/include)
find_path(DOCTEST_DOCTEST_DIR NAMES doctest/doctest.h PATHS ${DOCTEST_DOCTEST_DIRS})
if (DOCTEST_DOCTEST_DIR STREQUAL "DOCTEST_DOCTEST_DIR-NOTFOUND")
    include(FetchContent)
    FetchContent_Populate(
      doctest
      GIT_REPOSITORY https://github.com/doctest/doctest.git
    )
    list(APPEND CMAKE_MODULE_PATH "${doctest_SOURCE_DIR}/scripts/cmake/")
    set(DOCTEST_DOCTEST_DIR ${doctest_SOURCE_DIR})
else()
    set(doctest_BINARY_DIR ${CMAKE_BINARY_DIR}/doctest-build)
endif()

add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${DOCTEST_DOCTEST_DIR})
add_library(doctest::doctest ALIAS doctest)
