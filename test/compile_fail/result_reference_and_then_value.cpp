// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Result<T&>::AndThen() with a function that returns a plain value, which Map()
// is for.
//
// Expected: AndThen: f returns a Status or a Result

#include <utl/result.h>

void Case() {
  int target{0};
  static_cast<void>(
      utl::Result<int&>{target}.AndThen([](int& v) { return v; }));
}
