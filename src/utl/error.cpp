// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utl/error.h"

#include <stddef.h>
#include <stdint.h>

#include "utl/messages.h"

namespace utl {
namespace {

/*
 * Prints `code` in decimal, at most three digits. Not Print::print(code),
 * which brings in the core's printing of an unsigned long, with a 32-bit
 * division: in Basics it cost more than the texts it stands in for. Nor a
 * division of its own: `code / 100` pulled in two of libgcc's division
 * routines, 80 bytes. Counting the hundreds and tens off takes at most two
 * and nine rounds.
 */
size_t PrintCode(Print& out, uint8_t code) {
  constexpr uint8_t kHundred{100};
  constexpr uint8_t kTen{10};
  uint8_t hundreds{0};
  while (code >= kHundred) {
    code = static_cast<uint8_t>(code - kHundred);
    ++hundreds;
  }
  uint8_t tens{0};
  while (code >= kTen) {
    code = static_cast<uint8_t>(code - kTen);
    ++tens;
  }
  size_t printed{0};
  if (hundreds != 0) {
    printed += out.write(static_cast<uint8_t>('0' + hundreds));
  }
  if (hundreds != 0 || tens != 0) {
    printed += out.write(static_cast<uint8_t>('0' + tens));
  }
  printed += out.write(static_cast<uint8_t>('0' + code));
  return printed;
}

}  // namespace

__FlashStringHelper const* Error::Message() const {
  __FlashStringHelper const* const text{Messages::Find(group_, code_)};
  return text != nullptr ? text : F("");
}

// Not Printable, whose vptr would make an Error 5 bytes rather than 3.
size_t Error::PrintTo(Print& out) const {
  size_t count{out.print(group_.Name())};
  count += out.print(F(": "));
  __FlashStringHelper const* const text{Messages::Find(group_, code_)};
  if (text != nullptr) {
    count += out.print(text);
  } else {
    count += PrintCode(out, code_);
  }
  return count;
}

}  // namespace utl
