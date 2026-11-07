// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bus_error.h"

#include <avr/pgmspace.h>
#include <utl/array.h>
#include <utl/messages.h>

namespace bus {

char const kBusErrorGroup[] PROGMEM = "bus";

namespace {

char const kNackText[] PROGMEM = "nack";
char const kTimeoutText[] PROGMEM = "timeout";

// In the order of BusError.
constexpr auto kMessages PROGMEM = utl::make_array(kNackText, kTimeoutText);

}  // namespace

void UseBusMessages() {
  // The group by its name, a constant: a static whose initializer calls a
  // function would be initialized at run time, with a guard.
  static utl::Messages messages{utl::ErrorGroup{kBusErrorGroup}, kMessages};
  utl::UseMessages(messages);
}

}  // namespace bus
