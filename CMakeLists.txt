cmake_minimum_required(VERSION 3.10)
project(idbutil)
set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_find")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(DARWIN TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(FREEBSD TRUE)
endif()

# checking if we are called in the correct way:
#  with a -B argument.  and without a cache file in the source directory.
if (CMAKE_CACHEFILE_DIR STREQUAL "${CMAKE_SOURCE_DIR}")
    message(FATAL_ERROR "\nUnexpected CMakeCache.txt file in the source directory. Please remove it.")
    return()
endif()

if (EXISTS ${CMAKE_BINARY_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "\nRun cmake with an explicit -B buildpath")
    return()
endif()

if(MSVC)
    # /MP = multithreaded build
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    # /utf-8 = utf8 source and execution
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

if (WIN32)
    add_definitions(-DNOMINMAX -DNOGDI -DWIN32_LEAN_AND_MEAN)
endif()

find_package(idasdk REQUIRED)
find_package(cpputils REQUIRED)

add_library(idbutil INTERFACE)
target_include_directories(idbutil INTERFACE .)

add_executable(idbtool idbtool.cpp)
target_link_libraries(idbtool PRIVATE idasdk idbutil cpputils)


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    find_package(doctest REQUIRED)
    file(GLOB UnittestSrc tests/*.cpp)
    add_executable(unittests ${UnittestSrc})
    target_link_libraries(unittests PRIVATE cpputils idbutil doctest::doctest)
    target_compile_definitions(unittests PRIVATE WITH_DOCTEST)

    include(CTest)
    include(doctest)
    doctest_discover_tests(unittests)
endif()

