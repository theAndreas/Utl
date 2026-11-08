// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Result::InspectError() with a function that returns a Result, which would be
// dropped unseen.
//
// Expected: InspectError: f returns nothing

#include <utl/result.h>

void Case() {
  static_cast<void>(utl::Result<int>{1}.InspectError(
      [](utl::Error const&) { return utl::Result<int>{2}; }));
}
