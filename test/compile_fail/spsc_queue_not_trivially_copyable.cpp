// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A spsc_queue of elements that are not trivially copyable, whose copy would
// run code of its own in an interrupt.
//
// Expected: spsc_queue: T is trivially copyable

#include <utl/spsc_queue.h>

struct Owner {
  Owner(Owner const& other);
  int* owned;
};

void Case() {
  utl::spsc_queue<Owner, 4> const queue;
  static_cast<void>(queue);
}
