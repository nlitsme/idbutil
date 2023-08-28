# include this file to get a full dump of all cmake variables to stdout.

function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        if (ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if (NOT MATCHED)
                continue()
            endif()
        endif() 
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()

list(APPEND InterestingProps AUTOGEN_ORIGIN_DEPENDS AUTOMOC_COMPILER_PREDEFINES AUTOMOC_MACRO_NAMES AUTOMOC_PATH_PREFIX BINARY_DIR BUILD_WITH_INSTALL_RPATH EXCLUDE_FROM_ALL EXPORT_COMPILE_COMMANDS FOLDER HEADER_SETS IMPORTED IMPORTED_GLOBAL INCLUDE_DIRECTORIES INSTALL_RPATH INSTALL_RPATH_USE_LINK_PATH INTERFACE_HEADER_SETS INTERFACE_INCLUDE_DIRECTORIES INTERFACE_LINK_LIBRARIES ISPC_HEADER_SUFFIX LINK_LIBRARIES PCH_INSTANTIATE_TEMPLATES PCH_WARN_INVALID POSITION_INDEPENDENT_CODE RULE_LAUNCH_CUSTOM SKIP_BUILD_RPATH SOURCES SOURCE_DIR UNITY_BUILD_BATCH_SIZE UNITY_BUILD_MODE)
include(CMakePrintHelpers)
function(dump_targets)
    get_directory_property(_tlist BUILDSYSTEM_TARGETS)
    message(STATUS "bs: ${_tlist}")
    foreach (t ${_tlist})
        message(STATUS "----  ${t}")
        get_target_property(_slist ${t} SOURCES)
        foreach (s ${_slist})
            message(STATUS "  ${s}")
        endforeach()
        cmake_print_properties(TARGETS ${t} PROPERTIES ${InterestingProps})
    endforeach()
endfunction()



