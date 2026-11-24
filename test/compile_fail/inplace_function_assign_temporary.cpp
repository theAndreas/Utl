// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// An assignment to a temporary inplace_function, which would change nothing
// (CodingStyle C-17).
//
// Expected: (no match for [^ ]*operator=|no viable overloaded '=')

#include <utl/inplace_function.h>

// Declared only, as the case is only compiled.
utl::inplace_function<void()> Make();

void Case() {
  utl::inplace_function<void()> const other;
  Make() = other;
}
