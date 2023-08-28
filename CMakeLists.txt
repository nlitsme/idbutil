cmake_minimum_required(VERSION 3.10)
project(idbutil)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_find")
include(boilerplate)

find_package(idasdk REQUIRED)
find_package(cpputils REQUIRED)
find_package(libgmp)

add_library(idblib INTERFACE)
target_include_directories(idblib INTERFACE include)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR BUILD_TOOLS)
    add_subdirectory(tools)
endif()
include(CTest)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING OR BUILD_ALL_TESTS)
    add_subdirectory(tests)
endif()


