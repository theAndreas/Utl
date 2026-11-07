// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utl/index_error.h"

#include <avr/pgmspace.h>

#include "utl/array.h"
#include "utl/messages.h"

namespace utl {

// The name of the group, declared in index_error.h without its text, as only
// one file may define it: a name in flash is a char array.
char const kIndexErrorGroup[] PROGMEM = "index";

namespace {

char const kOutOfRangeText[] PROGMEM = "out of range";
char const kFullText[] PROGMEM = "full";

// In the order of IndexError.
constexpr auto kMessages PROGMEM = make_array(kOutOfRangeText, kFullText);

}  // namespace

void UseIndexMessages() {
  // The group by its name, a constant: a static whose initializer calls a
  // function would be initialized at run time, with a guard.
  static Messages messages{ErrorGroup{kIndexErrorGroup}, kMessages};
  UseMessages(messages);
}

}  // namespace utl
