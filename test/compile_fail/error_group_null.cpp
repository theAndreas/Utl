// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A group of a null pointer, which would compare equal to the no group of an
// ok Status: a group is named by an array of chars.
//
// Expected: deleted (function|constructor)

#include <utl/error_group.h>

void Case() {
  utl::ErrorGroup const group{nullptr};
  static_cast<void>(group);
}
