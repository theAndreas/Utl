// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The errors of a bus, the second group of this example. Its codes overlap
// with those of the font, which is fine: an error compares group and code.

#ifndef UTL_EXAMPLES_BASICS_BUS_ERROR_H_
#define UTL_EXAMPLES_BASICS_BUS_ERROR_H_

#include <avr/pgmspace.h>
#include <stdint.h>
#include <utl/error_group.h>

namespace bus {

enum class BusError : uint8_t {
  kNack,
  kTimeout,
};

// "bus", defined in bus_error.cpp; its address is the group.
// A name in flash is a char array, declared here without its text, as only
// one file may define it.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
extern char const kBusErrorGroup[] PROGMEM;

constexpr utl::ErrorGroup GetErrorGroup(BusError) {
  return utl::ErrorGroup{kBusErrorGroup};
}

// Turns on the texts of the group: until then its errors print their codes.
void UseBusMessages();

}  // namespace bus

#endif  // UTL_EXAMPLES_BASICS_BUS_ERROR_H_
