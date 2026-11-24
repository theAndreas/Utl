// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Error of a temporary Status: the pointer to it would dangle at the end
// of the statement (CodingStyle C-16).
//
// Expected: deleted (member )?function [^(]*error

#include <utl/error.h>
#include <utl/status.h>

void Case() {
  utl::Error const* error{utl::Status{}.error()};
  static_cast<void>(error);
}
