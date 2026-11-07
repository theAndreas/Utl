// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Result::MapError() with a function that returns a Result: recovering is what
// OrElse() is for.
//
// Expected: MapError: f returns an Error or a code

#include <utl/result.h>

void Case() {
  static_cast<void>(utl::Result<int>{1}.MapError(
      [](utl::Error const&) { return utl::Result<int>{2}; }));
}
