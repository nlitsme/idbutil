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
#if !defined(USE_CATCH) && !defined(USE_DOCTEST)
#define USE_DOCTEST
#endif

#ifdef USE_CATCH
#ifdef UNITTESTMAIN
#define CATCH_CONFIG_MAIN 
#endif
//#define CATCH_CONFIG_ENABLE_TUPLE_STRINGMAKER
#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
#if __has_include(<catch2/catch.hpp>)
#include <catch2/catch.hpp>
#elif __has_include("single_include/catch.hpp")
#include "single_include/catch.hpp"
#elif __has_include("contrib/catch.hpp")
#include "contrib/catch.hpp"
#else
#error  "Could not find catch.hpp"
#endif

#define SKIPTEST  , "[!hide]"

// doctest has suites, catch doesn't.
#define TEST_SUITE(x)  namespace

#elif defined(USE_DOCTEST)
#ifdef UNITTESTMAIN
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#endif
#include <ostream>
#if __has_include(<doctest/doctest.h>)
#include <doctest/doctest.h>
#elif __has_include("contrib/doctest.h")
#include "contrib/doctest.h"
#elif __has_include("single_include/doctest.h")
#include "single_include/doctest.h"
#else
#error  "Could not find doctest.h"
#endif
#define SECTION(...) SUBCASE(__VA_ARGS__)
#define SKIPTEST  * doctest::skip(true)

#define CHECK_THAT(a, b) 
#else
#error define either USE_CATCH or USE_DOCTEST
#endif

#define IN_UNITTEST 1
