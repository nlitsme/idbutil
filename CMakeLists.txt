cmake_minimum_required(VERSION 3.10)
project(idbutil)
set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

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


find_path(IDASDK_PATH NAMES include/netnode.hpp PATHS $ENV{IDASDK} $ENV{HOME}/src/idasdk76)
if (IDASDK_PATH STREQUAL IDASDK_PATH-NOTFOUND)
    message(FATAL_ERROR "IDASDK not found.")
endif()
add_library(idasdk INTERFACE)
target_include_directories(idasdk INTERFACE ${IDASDK_PATH}/include)
target_compile_definitions(idasdk INTERFACE MAXSTR=1024)
if (LINUX)
    target_compile_definitions(idasdk INTERFACE __LINUX__=1)
elseif (DARWIN)
    target_compile_definitions(idasdk INTERFACE __MAC__=1)
elseif (WIN32)
    target_compile_definitions(idasdk INTERFACE __NT__=1)
endif()
# this prevents idasdk:fpro.h to redefine all stdio stuff to 'dont_use_XXX'
target_compile_definitions(idasdk INTERFACE USE_STANDARD_FILE_FUNCTIONS)
# this prevents idasdk:pro.h to redefine all string functions to 'dont_use_XXX'
target_compile_definitions(idasdk INTERFACE USE_DANGEROUS_FUNCTIONS)
# disallow obsolete sdk functions.
target_compile_definitions(idasdk INTERFACE NO_OBSOLETE_FUNCS)


add_library(idbutil INTERFACE)
target_include_directories(idbutil INTERFACE .)


file(GLOB CPPUTILS_DIRS cpputils  submodules/cpputils ../cpputils ../../cpputils)
find_path(CPPUTILS_PATH NAMES string-lineenum.h PATHS ${CPPUTILS_DIRS})
if(NOT CPPUTILS_PATH)
    include(FetchContent)
    FetchContent_Populate(cpputils
        GIT_REPOSITORY https://github.com/nlitsme/cpputils)
    set(CPPUTILS_PATH ${CMAKE_BINARY_DIR}/cpputils-src)
endif()
add_library(cpputils INTERFACE)
target_include_directories(cpputils INTERFACE ${CPPUTILS_PATH})


add_executable(idbtool idbtool.cpp)
target_link_libraries(idbtool PRIVATE idasdk idbutil cpputils)


file(GLOB UnittestSrc tests/*.cpp)
add_executable(unittests ${UnittestSrc})
target_link_libraries(unittests cpputils idbutil)
target_compile_definitions(unittests PRIVATE WITH_DOCTEST)

