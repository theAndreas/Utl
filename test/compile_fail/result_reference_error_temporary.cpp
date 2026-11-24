// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Error of a temporary Result of a reference: the pointer to it would
// dangle at the end of the statement (CodingStyle C-16).
//
// Expected: deleted (member )?function [^(]*error

#include <utl/error.h>
#include <utl/result.h>

void Case() {
  int object{1};
  utl::Error const* error{utl::Result<int&>{object}.error()};
  static_cast<void>(error);
}
