// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A flash_span of elements that are not trivially copyable, which cannot be
// read from flash as their bytes.
//
// Expected: flash_span: T is trivially copyable

#include <utl/flash_span.h>

struct Owner {
  Owner(Owner const& other);
  int* owned;
};

void Case() {
  utl::flash_span<Owner> const owners;
  static_cast<void>(owners);
}
