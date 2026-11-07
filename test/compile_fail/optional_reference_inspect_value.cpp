// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// optional<T&>::Inspect() with a function that returns a value, which would be
// dropped unseen.
//
// Expected: Inspect: f returns nothing

#include <utl/optional.h>

void Case() {
  int target{0};
  static_cast<void>(
      utl::optional<int&>{target}.Inspect([](int const& v) { return v; }));
}
