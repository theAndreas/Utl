// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A group of errors for the tests, whose codes overlap with the other's. It
// has no texts, so its errors print their codes.

#ifndef UTL_TEST_BETA_ERROR_H_
#define UTL_TEST_BETA_ERROR_H_

#include <avr/pgmspace.h>
#include <stdint.h>
#include <utl/error_group.h>

namespace utl_test {

enum class BetaError : uint8_t {
  kFirst,
};

// "beta", defined in beta_error.cpp; its address is the group.
// A name in flash is a char array, declared here without its text, as only
// one file may define it.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
extern char const kBetaErrorGroup[] PROGMEM;

constexpr utl::ErrorGroup GetErrorGroup(BetaError) {
  return utl::ErrorGroup{kBetaErrorGroup};
}

}  // namespace utl_test

#endif  // UTL_TEST_BETA_ERROR_H_
