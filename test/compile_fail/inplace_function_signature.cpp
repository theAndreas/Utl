// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// An inplace_function of a type that is no function, such as one of the
// result alone.
//
// Expected: inplace_function: Signature is a function type

#include <utl/inplace_function.h>

void Case() {
  utl::inplace_function<int> const f;
  static_cast<void>(f);
}
