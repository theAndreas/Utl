// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Result::Inspect() with a function that returns a value, which would be
// dropped unseen.
//
// Expected: Inspect: f returns nothing

#include <utl/result.h>

void Case() {
  static_cast<void>(utl::Result<int>{1}.Inspect([](int v) { return v; }));
}
