// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "alpha_error.h"

#include <avr/pgmspace.h>
#include <utl/array.h>
#include <utl/error.h>
#include <utl/messages.h>

namespace self_check {

char const kAlphaErrorGroup[] PROGMEM = "alpha";

namespace {

char const kFirstText[] PROGMEM = "alpha first";
char const kSecondText[] PROGMEM = "alpha second";

// In the order of AlphaError.
constexpr auto kMessages PROGMEM = utl::make_array(kFirstText, kSecondText);

}  // namespace

void UseAlphaMessages() {
  // The group by its name, a constant: a static whose initializer calls a
  // function would be initialized at run time, with a guard.
  static utl::Messages messages{utl::ErrorGroup{kAlphaErrorGroup}, kMessages};
  utl::UseMessages(messages);
}

utl::Error MakeAlphaSecondElsewhere() { return AlphaError::kSecond; }

}  // namespace self_check
