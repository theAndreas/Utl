// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A spsc_queue of 256 elements, which its counts of a byte and its mask cannot
// serve.
//
// Expected: spsc_queue: N is a power of two

#include <stdint.h>
#include <utl/spsc_queue.h>

void Case() {
  utl::spsc_queue<uint8_t, 256> const queue;
  static_cast<void>(queue);
}
