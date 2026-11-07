// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "beta_error.h"

#include <avr/pgmspace.h>
#include <utl/array.h>
#include <utl/messages.h>

namespace self_check {

char const kBetaErrorGroup[] PROGMEM = "beta";

namespace {

char const kFirstText[] PROGMEM = "beta first";

// In the order of BetaError.
constexpr auto kMessages PROGMEM = utl::make_array(kFirstText);

}  // namespace

void UseBetaMessages() {
  // The group by its name, a constant: a static whose initializer calls a
  // function would be initialized at run time, with a guard.
  static utl::Messages messages{utl::ErrorGroup{kBetaErrorGroup}, kMessages};
  utl::UseMessages(messages);
}

}  // namespace self_check
