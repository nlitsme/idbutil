find_package(doctest REQUIRED)

file(GLOB UnittestSrc *.cpp)
add_executable(idbutil_unittests ${UnittestSrc})
set_property(TARGET idbutil_unittests PROPERTY OUTPUT_NAME unittests)
target_link_libraries(idbutil_unittests PRIVATE cpputils idblib doctest::doctest)
target_compile_definitions(idbutil_unittests PRIVATE WITH_DOCTEST)

include(CTest)
include(doctest)
doctest_discover_tests(idbutil_unittests)
