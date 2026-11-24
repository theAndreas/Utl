// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// An element of a const temporary array, which no copy can be moved out of:
// the reference would dangle at the end of the statement (CodingStyle C-16).
//
// Expected: deleted (member function|function|operator) [^(]*\[\]

#include <stdint.h>
#include <utl/array.h>

// Declared only, as the case is only compiled.
utl::array<uint8_t, 3> const MakeRows();

void Case() {
  uint8_t const& first{MakeRows()[0]};
  static_cast<void>(first);
}
