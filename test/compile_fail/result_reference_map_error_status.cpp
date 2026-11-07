// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Result<T&>::MapError() with a function that returns a Status, which could be
// ok.
//
// Expected: MapError: f returns an Error or a code

#include <utl/result.h>

void Case() {
  int target{0};
  static_cast<void>(utl::Result<int&>{target}.MapError(
      [](utl::Error const&) { return utl::Status::Ok(); }));
}
