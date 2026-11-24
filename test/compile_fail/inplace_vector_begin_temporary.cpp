// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// An iterator into a temporary inplace_vector: it would dangle at the end of
// the statement (CodingStyle C-16).
//
// Expected: deleted (member )?function [^(]*begin

#include <stdint.h>
#include <utl/inplace_vector.h>

void Case() {
  uint8_t const* first{utl::inplace_vector<uint8_t, 3>{1, 2}.begin()};
  static_cast<void>(first);
}
