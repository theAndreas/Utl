// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utl/optional_error.h"

#include <avr/pgmspace.h>

#include "utl/array.h"
#include "utl/messages.h"

namespace utl {

// The name of the group, declared in optional_error.h without its text, as only
// one file may define it: a name in flash is a char array.
char const kOptionalErrorGroup[] PROGMEM = "optional";

namespace {

char const kEmptyText[] PROGMEM = "empty";

// In the order of OptionalError.
constexpr auto kMessages PROGMEM = make_array(kEmptyText);

}  // namespace

void UseOptionalMessages() {
  // The group by its name, a constant: a static whose initializer calls a
  // function would be initialized at run time, with a guard.
  static Messages messages{ErrorGroup{kOptionalErrorGroup}, kMessages};
  UseMessages(messages);
}

}  // namespace utl
