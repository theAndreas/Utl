// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The address of the value of a temporary optional: it would dangle at the end
// of the statement (CodingStyle C-16).
//
// Expected: deleted (member )?function [^(]*operator->

#include <utl/optional.h>

void Case() {
  int const* value{utl::optional<int>{1}.operator->()};
  static_cast<void>(value);
}
