// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The first of two groups for SelfCheck, whose codes overlap on purpose.

#ifndef UTL_EXAMPLES_SELFCHECK_ALPHA_ERROR_H_
#define UTL_EXAMPLES_SELFCHECK_ALPHA_ERROR_H_

#include <avr/pgmspace.h>
#include <stdint.h>
#include <utl/error.h>
#include <utl/error_group.h>

namespace self_check {

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

// Made in alpha_error.cpp, so that SelfCheck compares an error from another
// translation unit: equal only if both see the group at the same address.
utl::Error MakeAlphaSecondElsewhere();

}  // namespace self_check

#endif  // UTL_EXAMPLES_SELFCHECK_ALPHA_ERROR_H_
