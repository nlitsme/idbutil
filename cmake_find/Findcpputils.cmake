# NOTE: you can avoid downloading cpputils, by symlinking to a downloaded version here:
find_path(CPPUTILS_DIR NAMES string-lineenum.h PATHS symlinks/cpputils)
if(CPPUTILS_DIR STREQUAL "CPPUTILS_DIR-NOTFOUND")
    include(FetchContent)
    FetchContent_Populate(cpputils
        GIT_REPOSITORY https://github.com/nlitsme/cpputils)
    set(CPPUTILS_DIR ${cpputils_SOURCE_DIR})
else()
    set(cpputils_BINARY_DIR ${CMAKE_BINARY_DIR}/cpputils-build)
endif()

add_subdirectory(${CPPUTILS_DIR} ${cpputils_BINARY_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cpputils REQUIRED_VARS CPPUTILS_DIR)

