/*
 * catch is available from: https://github.com/catchorg/Catch2
 * doctest is available from https://github.com/onqtam/doctest
 *
 *
 * Doctest is almost a drop-in replacement for Catch.
 * Though Catch has a few more features, and works without any restrictions,
 * doctest has much faster compilation times, our 44 unittests build
 * takes about 13 minutes to build with catch, or about 3.5 minutes when
 * using doctest.
 *
 */
#if !defined(WITH_CATCH) && !defined(WITH_DOCTEST)
#define WITH_DOCTEST
#endif

#ifdef WITH_CATCH
#ifdef UNITTESTMAIN
#define CATCH_CONFIG_MAIN 
#endif
#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
#include "contrib/catch.hpp"

#define SKIPTEST  , "[!hide]"

// doctest has suites, catch doesn't.
#define TEST_SUITE(x)  namespace

#elif defined(WITH_DOCTEST)
#ifdef UNITTESTMAIN
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#endif
#include <ostream>
#include "contrib/doctest.h"
#define SECTION(...) SUBCASE(__VA_ARGS__)
#define SKIPTEST  * doctest::skip(true)

#define CHECK_THAT(a, b) 
#else
#error define either WITH_CATCH or WITH_DOCTEST
#endif

