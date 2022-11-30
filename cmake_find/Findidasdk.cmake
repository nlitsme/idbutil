# note: this depends on my local install
find_path(IDASDK_PATH NAMES include/netnode.hpp PATHS $ENV{IDASDK} $ENV{HOME}/src/idasdk_pro80 c:/local/idasdk77)
if (IDASDK_PATH STREQUAL "IDASDK_PATH-NOTFOUND")
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

