// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utl/misuse.h"

#include <avr/pgmspace.h>
#include <stddef.h>
#include <stdint.h>

#include "utl/array.h"

namespace utl {
namespace {

char const kResultValueText[] PROGMEM = "value() of a Result with an error";
char const kOptionalValueText[] PROGMEM = "* or -> of an empty optional";
char const kIndexText[] PROGMEM = "[] beyond the end";
char const kEmptyText[] PROGMEM = "front(), back() or pop_back() when empty";
char const kRemoveText[] PROGMEM =
    "remove_prefix() or remove_suffix() beyond the end";
char const kNullFunctionText[] PROGMEM = "function_ref of a null pointer";
char const kEmptyFunctionText[] PROGMEM = "call of an empty inplace_function";

// In the order of Misuse.
constexpr auto kTexts PROGMEM =
    make_array(kResultValueText, kOptionalValueText, kIndexText, kEmptyText,
               kRemoveText, kNullFunctionText, kEmptyFunctionText);

}  // namespace

size_t PrintMisuse(Print& out, Misuse misuse) {
  auto const index = static_cast<uint8_t>(misuse);
  size_t count{out.print(F("utl: "))};
  if (index < kTexts.size()) {
    count += out.print(
        static_cast<__FlashStringHelper const*>(pgm_read_ptr(&kTexts[index])));
  }
  return count;
}

}  // namespace utl
