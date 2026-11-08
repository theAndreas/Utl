// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "font_error.h"

#include <avr/pgmspace.h>
#include <utl/array.h>
#include <utl/messages.h>

namespace font {

char const kFontErrorGroup[] PROGMEM = "font";

namespace {

char const kUnknownCharText[] PROGMEM = "unknown char";
char const kNullTextText[] PROGMEM = "null text";

// In the order of FontError.
constexpr auto kMessages PROGMEM =
    utl::make_array(kUnknownCharText, kNullTextText);

}  // namespace

void UseFontMessages() {
  // The group by its name, a constant: a static whose initializer calls a
  // function would be initialized at run time, with a guard.
  static utl::Messages messages{utl::ErrorGroup{kFontErrorGroup}, kMessages};
  utl::UseMessages(messages);
}

}  // namespace font
