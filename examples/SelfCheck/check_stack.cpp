// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// How deep the stack went in this run: PaintStack() fills the free RAM below
// the stack with a pattern as setup() begins, and CheckStack() finds at the end
// the lowest byte the stack wrote over. It sees what this run calls and the
// interrupts that came, not every path there is.

#include <Arduino.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/atomic.h>

#include "self_check.h"

namespace self_check {
namespace {

// A byte the stack wrote with this very value is missed, and the depth found
// is that much less; 0xC5 is no 0, 0xFF or small number, which it writes most.
// A char, as avr-libc gives the free RAM as one.
constexpr char kPaint{static_cast<char>(0xC5)};

// SelfCheck took 202 bytes when this was written; the rest is room to grow.
constexpr uint16_t kStackBudget{256};

// The byte of RAM at `address`: SP and RAMEND are numbers.
char* InRam(uint16_t address) {
  // An address the hardware gives as a number; only this cast makes it one.
  // NOLINTNEXTLINE(performance-no-int-to-ptr,cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<char*>(address);
}

// The first byte above .bss, where the free RAM begins, as avr-libc's malloc()
// knows it.
char* FreeBegin() { return __malloc_heap_start; }

}  // namespace

void PaintStack() {
  // The interrupts off, as one would push onto what is painted.
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    // Up to the stack pointer, the next byte a push writes; above it lie the
    // frames of the calls so far, which count as taken.
    char const* const top{InRam(SP)};
    char* paint{FreeBegin()};
    while (paint < top) {
      *paint = kPaint;
      ++paint;
    }
  }
}

void CheckStack() {
  char const* const top{InRam(SP)};
  char const* lowest{FreeBegin()};
  while ((lowest < top) && (*lowest == kPaint)) {
    ++lowest;
  }
  auto const depth{static_cast<uint16_t>(InRam(RAMEND) - lowest + 1)};
  Check(depth < kStackBudget, F("the stack under 256 bytes"));
}

}  // namespace self_check
