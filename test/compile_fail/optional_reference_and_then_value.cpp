// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// optional<T&>::AndThen() with a function that returns a plain value, which
// Map() is for.
//
// Expected: AndThen: f returns an optional

#include <utl/optional.h>

void Case() {
  int target{0};
  static_cast<void>(
      utl::optional<int&>{target}.AndThen([](int& v) { return v; }));
}
