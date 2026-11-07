# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# The tests on the host: the library against a mock of the Arduino core, on
# GoogleTest, as built by default and with UTL_CHECK_ACCESS, as SelfCheck runs
# in simavr as well. The library is built as gnu++11, as avr-gcc builds it;
# only the tests are C++17, which GoogleTest needs.
#
#   cmake --workflow --preset default    # the tests
#   cmake --workflow --preset sanitize   # under ASan and UBSan
#   tools/coverage.sh                    # the tests, and what they reach

cmake_minimum_required(VERSION 3.25)
project(Utl LANGUAGES CXX)
enable_testing()
# For clangd, which reads it through .clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(UTL_COVERAGE "Build with --coverage, for gcovr" OFF)
option(UTL_SANITIZE "Build with AddressSanitizer and UBSan" OFF)

find_package(GTest REQUIRED)
include(GoogleTest)

set(warnings -Wall -Wextra -Werror)
set(instrument)
if(UTL_COVERAGE)
  list(APPEND instrument --coverage)
endif()
if(UTL_SANITIZE)
  list(APPEND instrument -fsanitize=address,undefined
       -fno-sanitize-recover=all -fno-omit-frame-pointer)
endif()

file(GLOB library_sources CONFIGURE_DEPENDS src/utl/*.cpp)
set(test_sources
    test/array_test.cpp
    test/charconv_test.cpp
    test/constant_wrapper_test.cpp
    test/error_test.cpp
    test/flash_span_test.cpp
    test/flash_string_view_test.cpp
    test/function_ref_test.cpp
    test/inplace_function_test.cpp
    test/inplace_vector_test.cpp
    test/misuse_test.cpp
    test/optional_reference_test.cpp
    test/optional_test.cpp
    test/reference_wrapper_test.cpp
    test/result_reference_test.cpp
    test/result_test.cpp
    test/span_test.cpp
    test/status_test.cpp
    test/string_view_test.cpp
    test/alpha_error.cpp
    test/beta_error.cpp)

# One library and one test program per variant: the switches change what is
# compiled, so each variant is a build of its own.
function(add_variant name)
  set(defines ${ARGN})

  add_library(utl_${name} STATIC ${library_sources} test/mock/Arduino.cpp)
  target_include_directories(utl_${name} PUBLIC src test/mock)
  target_compile_definitions(utl_${name} PUBLIC ${defines})
  target_compile_options(utl_${name} PRIVATE -std=gnu++11 ${warnings}
                                              ${instrument})
  target_link_options(utl_${name} PUBLIC ${instrument})

  add_executable(utl_test_${name} ${test_sources})
  target_compile_features(utl_test_${name} PRIVATE cxx_std_17)
  # The root, so that a test includes its helpers as "test/...", as Google
  # wants an include to name the directory.
  target_include_directories(utl_test_${name} PRIVATE ${CMAKE_SOURCE_DIR})
  target_compile_options(utl_test_${name} PRIVATE ${warnings} ${instrument})
  target_link_libraries(utl_test_${name} PRIVATE utl_${name} GTest::gtest_main)
  gtest_discover_tests(utl_test_${name} TEST_PREFIX ${name}/)
endfunction()

add_variant(default)
add_variant(checked UTL_CHECK_ACCESS=1)

# What must not compile: each file in test/compile_fail/ uses utl wrongly, and
# its test passes only if the compiler stops it with the message of the
# static_assert that its line "// Expected: ..." names. A static_assert that
# went missing, or another error, fails it. The compiler is called directly,
# with -fsyntax-only and as gnu++11, as avr-gcc builds a sketch: no target,
# so that tests run in parallel never build in the same tree at once.
file(GLOB compile_fail_cases CONFIGURE_DEPENDS test/compile_fail/*.cpp)
foreach(case IN LISTS compile_fail_cases)
  get_filename_component(name ${case} NAME_WE)
  file(STRINGS ${case} expected REGEX "^// Expected: " LIMIT_COUNT 1)
  string(REGEX REPLACE "^// Expected: " "" expected "${expected}")
  if(expected STREQUAL "")
    message(FATAL_ERROR "${case} names no message: a line \"// Expected: \".")
  endif()
  add_test(NAME compile_fail/${name}
           COMMAND ${CMAKE_CXX_COMPILER} -std=gnu++11 -fsyntax-only
                   -I${CMAKE_SOURCE_DIR}/src -I${CMAKE_SOURCE_DIR}/test/mock
                   ${case})
  # GCC says "static assertion failed: <message>", Clang "static assertion
  # failed due to requirement '...': <message>"; both on one line.
  set_tests_properties(
    compile_fail/${name}
    PROPERTIES PASS_REGULAR_EXPRESSION "static assertion failed[^\n]*${expected}")
endforeach()
