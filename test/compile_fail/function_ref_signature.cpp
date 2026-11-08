// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A function_ref of a type that is no function, such as one of the result
// alone.
//
// Expected: function_ref: Signature is a function type

#include <utl/function_ref.h>

void Case() {
  int const value{1};
  utl::function_ref<int> const f{value};
  static_cast<void>(f);
}
