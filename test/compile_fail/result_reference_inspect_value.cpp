// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Result<T&>::Inspect() with a function that returns a value, which would be
// dropped unseen.
//
// Expected: Inspect: f returns nothing

#include <utl/result.h>

void Case() {
  int target{0};
  static_cast<void>(
      utl::Result<int&>{target}.Inspect([](int const& v) { return v; }));
}
