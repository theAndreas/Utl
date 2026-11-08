// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The second of two groups for SelfCheck, whose codes overlap on purpose.

#ifndef UTL_EXAMPLES_SELFCHECK_BETA_ERROR_H_
#define UTL_EXAMPLES_SELFCHECK_BETA_ERROR_H_

#include <avr/pgmspace.h>
#include <stdint.h>
#include <utl/error_group.h>

namespace self_check {

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

// Turns on the texts of the group: until then its errors print their codes.
void UseBetaMessages();

}  // namespace self_check

#endif  // UTL_EXAMPLES_SELFCHECK_BETA_ERROR_H_
