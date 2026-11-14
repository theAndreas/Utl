// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A value that turns negative in an element type of its width, 0xFFFF for an
// int16_t, and comes back as it was: make_array<T>() has to stop the build
// rather than make it -1.

#include <stdint.h>
#include <utl/array.h>

constexpr auto kTable = utl::make_array<int16_t>(uint16_t{0xFFFF});
