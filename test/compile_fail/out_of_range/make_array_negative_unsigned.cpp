// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A negative value for an element type without a sign, -1 for a uint8_t:
// make_array<T>() has to stop the build rather than wrap it to 255.

#include <stdint.h>
#include <utl/array.h>

constexpr auto kTable = utl::make_array<uint8_t>(1, -1);
