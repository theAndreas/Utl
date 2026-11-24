// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The address of the elements of a temporary array: the pointer would dangle
// at the end of the statement (CodingStyle C-16).
//
// Expected: deleted (member )?function [^(]*data

#include <stdint.h>
#include <utl/array.h>

void Case() {
  uint8_t const* data{utl::array<uint8_t, 3>{{1, 2, 3}}.data()};
  static_cast<void>(data);
}
