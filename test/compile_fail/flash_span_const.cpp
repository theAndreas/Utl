// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A flash_span of const elements, which says nothing more: flash cannot be
// written anyway.
//
// Expected: flash_span: T is not const

#include <stdint.h>
#include <utl/flash_span.h>

void Case() {
  utl::flash_span<uint8_t const> const rows;
  static_cast<void>(rows);
}
