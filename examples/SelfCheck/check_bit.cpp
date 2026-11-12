// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Bit, where the AVR differs from the host: an int of 16 bits, which a mask of
// a uint16_t must not be shifted in, and a register as the target of SetBits(),
// ClearBits() and WriteBitGroup().

#include <Utl.h>
#include <avr/io.h>
#include <stdint.h>

#include "self_check.h"

namespace self_check {

void CheckBit() {
  uint16_t const volatile top_of_16{0x8000U};
  uint32_t const volatile top_of_32{0x80000000UL};

  uint16_t const value_16{top_of_16};
  bool const read_16{utl::Bit<uint16_t>::ReadBit(value_16, 15)};
  bool const cleared_16{utl::Bit<uint16_t>::ClearBit(value_16, 15) == 0U};
  bool const group_16{utl::Bit<uint16_t>::ReadBitGroup(value_16, 0x0F, 12) ==
                      0x08U};
  Check(read_16 && cleared_16 && group_16,
        F("Bit<uint16_t> reads, clears and shifts down its top bit"));

  uint32_t const value_32{top_of_32};
  Check(utl::Bit<uint32_t>::ToMask(31) == value_32,
        F("Bit<uint32_t> makes the mask of its top bit"));

  GPIOR0 = 0x81;
  utl::Bit<uint8_t>::SetBits(GPIOR0, 0x0C);
  bool const set{GPIOR0 == 0x8D};
  utl::Bit<uint8_t>::ClearBits(GPIOR0, 0x81);
  bool const cleared{GPIOR0 == 0x0C};
  utl::Bit<uint8_t>::WriteBitGroup(GPIOR0, 0xF0, 0xA5);
  bool const written{GPIOR0 == 0xAC};
  Check(set && cleared && written,
        F("SetBits, ClearBits and WriteBitGroup on a register"));
}

}  // namespace self_check
