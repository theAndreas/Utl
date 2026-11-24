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
#
# Configured with cmake/toolchains/avr-gcc.cmake, it builds the examples for the AVR instead, as
# the target cmake of tools/project.sh: see the branch of CMAKE_CROSSCOMPILING below.

cmake_minimum_required(VERSION 3.25)
project(Utl LANGUAGES CXX)
enable_testing()
# For clangd, which reads it through .clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The warnings, the instrumentation of the presets sanitize and coverage, and the tests of
# what must not compile come from the template.
include(cmake/checks.cmake)

file(GLOB library_sources CONFIGURE_DEPENDS src/utl/*.cpp)

# The target cmake of tools/project.sh: the examples for the AVR, by avr-gcc from apt, and the
# Arduino core with them, compiled here as arduino:avr's platform.txt compiles it, so that
# avr-gcc writes the call graphs make stack adds up (tools/stack.sh). Each example is
# programs/<Name>.elf, named in programs.txt, as tools/lib/cmake.sh reads them.
if(CMAKE_CROSSCOMPILING)
  enable_language(C ASM)

  set(ARDUINO_CORE_DIR "" CACHE PATH "arduino:avr as arduino-cli installs it")
  set(ARDUINO_VARIANT "" CACHE STRING "The folder of variants/ the board takes its pins from")
  set(core_dir ${ARDUINO_CORE_DIR}/cores/arduino)
  set(variant_dir ${ARDUINO_CORE_DIR}/variants/${ARDUINO_VARIANT})
  if(NOT IS_DIRECTORY ${core_dir} OR NOT IS_DIRECTORY ${variant_dir})
    message(FATAL_ERROR "No Arduino core at ${core_dir} and ${variant_dir}: make setup installs "
                        "it, and tools/project.sh names it in ARDUINO_CORE_DIR.")
  endif()

  # The flags of the variant, as variant_flags() of tools/project.sh prints them, on every C++
  # file, the core's as well, as arduino-cli passes them in compiler.cpp.extra_flags: a file
  # compiled without the plugin of the variant flash would read a vtable in flash from RAM. Not
  # on a C file, into whose compiler the plugin of C++ does not load.
  set(VARIANT_FLAGS "" CACHE STRING "The flags of a variant, as variant_flags() prints them")
  separate_arguments(variant_flags UNIX_COMMAND "${VARIANT_FLAGS}")
  foreach(flag IN LISTS variant_flags)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${flag}>)
  endforeach()
  # As platform.txt of arduino:avr 1.8.8 has them, IDE 1.6.7 as the version, which arduino-cli
  # passes as well. Linked with -flto as there, its call graphs at the link, beside the program.
  add_compile_definitions(ARDUINO=10607 ARDUINO_AVR_UNO ARDUINO_ARCH_AVR)
  add_compile_options(-flto $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
                      $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>)
  add_link_options(-flto -save-temps=obj)
  set(CMAKE_C_STANDARD 11)
  set(CMAKE_C_EXTENSIONS ON)

  # The core is the toolchain's own code: compiled without warnings, as arduino-cli leaves its
  # warnings out as well. An archive, so that the linker takes only what is called: its
  # interrupt handlers, as Tone's, would clash with a sketch's otherwise. Packed by avr-gcc-ar,
  # which indexes the symbols of an object of -flto, as avr-ar does not.
  set(CMAKE_AR ${CMAKE_CXX_COMPILER_AR})
  set(CMAKE_RANLIB ${CMAKE_CXX_COMPILER_RANLIB})
  file(GLOB core_sources CONFIGURE_DEPENDS ${core_dir}/*.c ${core_dir}/*.cpp ${core_dir}/*.S)
  add_library(arduino_core STATIC ${core_sources})
  target_include_directories(arduino_core SYSTEM PUBLIC ${core_dir} ${variant_dir})
  target_compile_options(arduino_core PRIVATE -w $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++11>
                                              $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>)

  # Each folder of examples/, not its files, as .clang-tidy and CPPLINT.cfg lie there as well.
  # The sketch is C++ with Arduino.h first, as arduino-cli makes it.
  file(GLOB examples LIST_DIRECTORIES true CONFIGURE_DEPENDS examples/*)
  set(programs "")
  foreach(dir IN LISTS examples)
    if(NOT IS_DIRECTORY ${dir})
      continue()
    endif()
    get_filename_component(name ${dir} NAME)
    string(APPEND programs "${name}\n")
    file(GLOB sources CONFIGURE_DEPENDS ${dir}/*.cpp)
    set_source_files_properties(${dir}/${name}.ino PROPERTIES LANGUAGE CXX
                                COMPILE_OPTIONS "-include;Arduino.h")
    add_executable(${name} ${dir}/${name}.ino ${sources} ${library_sources})
    target_include_directories(${name} PRIVATE src ${dir})
    target_compile_options(${name} PRIVATE -std=gnu++11 ${CHECK_WARNINGS})
    target_link_libraries(${name} PRIVATE arduino_core m)
    # .elf here rather than as CMAKE_EXECUTABLE_SUFFIX in the toolchain file, which CMake's
    # platform file Generic sets back to nothing.
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/programs
                                             SUFFIX .elf)
  endforeach()
  file(WRITE ${CMAKE_BINARY_DIR}/programs.txt "${programs}")
  return()
endif()

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

set(test_sources
    test/array_test.cpp
    test/bit_test.cpp
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
    test/spsc_queue_test.cpp
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
  target_compile_options(utl_${name} PRIVATE -std=gnu++11 ${CHECK_WARNINGS}
                                              ${CHECK_INSTRUMENT})
  target_link_options(utl_${name} PUBLIC ${CHECK_INSTRUMENT})

  add_executable(utl_test_${name} ${test_sources})
  target_compile_features(utl_test_${name} PRIVATE cxx_std_17)
  # The root, so that a test includes its helpers as "test/...", as Google
  # wants an include to name the directory.
  target_include_directories(utl_test_${name} PRIVATE ${CMAKE_SOURCE_DIR})
  target_compile_options(utl_test_${name} PRIVATE ${CHECK_WARNINGS}
                                                   ${CHECK_INSTRUMENT})
  target_link_libraries(utl_test_${name} PRIVATE utl_${name} GTest::gtest_main)
  gtest_discover_tests(utl_test_${name} TEST_PREFIX ${name}/)
endfunction()

add_variant(default)
add_variant(checked UTL_CHECK_ACCESS=1)

# What must not compile: each file in test/compile_fail/ uses utl wrongly, and its test passes
# only if the compiler stops it with what its line "// Expected: ..." names: the message of a
# static_assert, or ValueOutOfRange, which make_array<T>() calls for a value that does not fit T
# to stop the constant evaluation. In gnu++11, as avr-gcc builds a sketch, and in C++17, as the
# tests are.
add_compile_fail_tests(DIRECTORY ${CMAKE_SOURCE_DIR}/test/compile_fail STANDARDS gnu++11 c++17
                       INCLUDES ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/test/mock)
