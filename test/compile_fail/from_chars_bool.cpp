// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// from_chars() into a bool, which the standard leaves out as well.
//
// Expected: from_chars: T is an integer other than bool

#include <utl/charconv.h>

void Case() {
  char const text[] = "1";
  bool value{false};
  static_cast<void>(utl::from_chars(text, text + 1, value));
}
