// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A value below what a signed element type holds, -129 for an int8_t:
// make_array<T>() has to stop the build rather than wrap it to 127.

#include <stdint.h>
#include <utl/array.h>

constexpr auto kTable = utl::make_array<int8_t>(-128, -129);
