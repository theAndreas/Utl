// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// optional<T&>::ValueOrElse() with a function that returns a T made in it,
// which would be gone before the caller got to it.
//
// Expected: f returns a T&

#include <utl/optional.h>

void Case() {
  int target{0};
  static_cast<void>(utl::optional<int&>{target}.ValueOrElse([] { return 1; }));
}
