// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// optional::AndThen() with a function that returns a plain value, which Map()
// is for.
//
// Expected: AndThen: f returns an optional

#include <utl/optional.h>

void Case() {
  static_cast<void>(utl::optional<int>{1}.AndThen([](int v) { return v; }));
}
