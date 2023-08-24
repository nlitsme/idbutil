if (TARGET idbutil)
    return()
endif()
find_path(IDBUTIL_DIR NAMES include/idblib/idb3.h PATHS symlinks/idbutil)
if(IDBUTIL_DIR STREQUAL "IDBUTIL_DIR-NOTFOUND")
    include(FetchContent)
    FetchContent_Populate(idbutil
        GIT_REPOSITORY https://github.com/nlitsme/idbutil)
    set(IDBUTIL_DIR ${idbutil_SOURCE_DIR})
else()
    set(idbutil_BINARY_DIR ${CMAKE_BINARY_DIR}/idbutil-build)
endif()

add_subdirectory(${IDBUTIL_DIR} ${idbutil_BINARY_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(idbutil REQUIRED_VARS IDBUTIL_DIR)


