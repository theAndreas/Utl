// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// flash_span: elements read from flash. The tables lie in PROGMEM only, so a
// read from RAM at their address gives other values, which the checks see.

#include <Utl.h>
#include <avr/pgmspace.h>
#include <stdint.h>

#include "self_check.h"

namespace self_check {
namespace {

// One table per width read apart: 1, 2 and 4 bytes, and 3, by memcpy_P().
struct Color {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr uint8_t kRows[] PROGMEM = {0x3c, 0x42, 0xa5, 0x81};
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr uint16_t kSteps[] PROGMEM = {100, 2000, 40000};
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr int32_t kOffsets[] PROGMEM = {-70000, 70000};
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr Color kColors[] PROGMEM = {{255, 0, 0}, {0, 128, 255}};
constexpr utl::array<uint8_t, 3> kArray PROGMEM = {{7, 8, 9}};

// XORs the rows: each read from flash in a range-based for.
uint8_t Xor(utl::flash_span<uint8_t> rows) {
  uint8_t result = 0;
  for (uint8_t const row : rows) {
    result ^= row;
  }
  return result;
}

}  // namespace

void CheckFlashSpan() {
  utl::flash_span<uint8_t> const rows(kRows);
  utl::flash_span<uint16_t> const steps(kSteps);
  utl::flash_span<int32_t> const offsets(kOffsets);
  utl::flash_span<Color> const colors(kColors);

  Check(rows.size() == 4 && rows[2] == 0xa5 && rows.front() == 0x3c &&
            rows.back() == 0x81,
        F("flash_span reads bytes from flash"));
  Check(steps[2] == 40000 && offsets[0] == -70000 && offsets[1] == 70000,
        F("flash_span reads words and double words"));
  Check(colors[1].green == 128 && colors[1].blue == 255 && colors[0].red == 255,
        F("flash_span reads a struct"));
  Check(Xor(rows) == (0x3cU ^ 0x42U ^ 0xa5U ^ 0x81U),
        F("flash_span in a range-based for"));
  Check(Xor(utl::flash_span<uint8_t>(kArray)) == (7U ^ 8U ^ 9U),
        F("flash_span of a utl::array"));
  Check(rows.at(3).ValueOr(0) == 0x81 &&
            rows.at(4).status() == utl::IndexError::kOutOfRange,
        F("flash_span::at"));
  Check(rows.subspan(1, 2).Map(Xor).ValueOr(0) == (0x42U ^ 0xa5U) &&
            rows.first(5).status() == utl::IndexError::kOutOfRange,
        F("flash_span::subspan, first"));
}

}  // namespace self_check
