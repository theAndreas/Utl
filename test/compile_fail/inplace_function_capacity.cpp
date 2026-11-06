// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// An inplace_function of a lambda larger than its Capacity, which would need
// the heap.
//
// Expected: inplace_function: the callable is larger than Capacity

#include <stdint.h>
#include <utl/inplace_function.h>

void Case() {
  int32_t const a = 1;
  int32_t const b = 2;
  utl::inplace_function<int32_t(), sizeof(a)> const f = [a, b] {
    return a + b;
  };
  static_cast<void>(f);
}
