if (${gmp_FOUND})
    return()
endif()
if (TARGET gmp)
    return()
endif()

find_path(GMPINC_DIR NAMES gmp.h gmpxx.h PATHS /usr/include /usr/local/include)
find_library(GMPLIB_DIR NAMES gmp libgmp PATHS /usr/lib /usr/local/lib)

if(NOT GMPINC_DIR STREQUAL "GMPINC_DIR-NOTFOUND" AND NOT GMPLIB_DIR STREQUAL "GMPLIB_DIR-NOTFOUND")
    add_library(gmp INTERFACE)
    target_link_libraries(gmp INTERFACE ${GMPLIB_DIR})
    target_include_directories(gmp INTERFACE ${GMPINC_DIR})
    target_compile_definitions(gmp INTERFACE HAVE_LIBGMP)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gmp REQUIRED_VARS GMPINC_DIR GMPLIB_DIR)


