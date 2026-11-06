// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// An inplace_function that returns a reference, which a call of an empty one
// could not give.
//
// Expected: inplace_function: R is void or has a default constructor

#include <utl/inplace_function.h>

void Case() {
  utl::inplace_function<int&()> const f;
  static_cast<void>(f);
}
