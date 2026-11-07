// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A group of errors for the tests, whose codes overlap with the other's. Its
// texts are turned on by the tests that print it.

#ifndef UTL_TEST_ALPHA_ERROR_H_
#define UTL_TEST_ALPHA_ERROR_H_

#include <avr/pgmspace.h>
#include <stdint.h>
#include <utl/error.h>
#include <utl/error_group.h>

namespace utl_test {

enum class AlphaError : uint8_t {
  kFirst,
  kSecond,
};

// "alpha", defined in alpha_error.cpp; its address is the group.
// A name in flash is a char array, declared here without its text, as only
// one file may define it.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
extern char const kAlphaErrorGroup[] PROGMEM;

constexpr utl::ErrorGroup GetErrorGroup(AlphaError) {
  return utl::ErrorGroup{kAlphaErrorGroup};
}

// Turns on the texts of the group: until then its errors print their codes.
void UseAlphaMessages();

// Made in alpha_error.cpp, so that a test compares an error from another
// translation unit: equal only if both see the group at the same address.
utl::Error MakeAlphaSecondElsewhere();

}  // namespace utl_test

#endif  // UTL_TEST_ALPHA_ERROR_H_
