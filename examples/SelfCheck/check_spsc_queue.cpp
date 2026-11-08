// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// spsc_queue between an interrupt and the code it interrupts: Timer2, which
// nothing else here uses, pushes counted ticks, and the check pops them,
// without turning interrupts off. A tick that finds the queue full is pushed
// again at the next interrupt, so none is lost, and they come in order.

#include <Utl.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <stdint.h>

#include "self_check.h"

namespace self_check {
namespace {

constexpr uint8_t kTicks{200};

utl::spsc_queue<uint8_t, 8> ticks;
uint8_t volatile next_tick{0};  // the interrupt's own: the one to push next

}  // namespace

// Every 400 cycles, 25 us at 16 MHz: pushes the next tick, until all are.
void PushTick() {
  if (next_tick != kTicks && ticks.push(next_tick).IsOk()) {
    next_tick = static_cast<uint8_t>(next_tick + 1);
  }
}

void CheckSpscQueue() {
  uint8_t const saved_a{TCCR2A};
  uint8_t const saved_b{TCCR2B};
  uint8_t const saved_compare{OCR2A};
  uint8_t const saved_mask{TIMSK2};
  TCCR2A = _BV(WGM21);  // CTC: counts to OCR2A, then from 0
  TCCR2B = _BV(CS21);   // a tick every 8 cycles
  OCR2A = 49;
  TIMSK2 = _BV(OCIE2A);

  uint8_t expected{0};
  bool in_order{true};
  bool was_full{false};
  uint32_t spins{0};
  while (expected != kTicks && spins < 1000000UL) {
    ++spins;
    if (expected == kTicks / 2 && !was_full) {
      // Halfway, let the interrupt fill it, and wait for it to find it full.
      while (!ticks.full() && spins < 1000000UL) {
        ++spins;
      }
      was_full = ticks.full();
    }
    utl::optional<uint8_t> const tick{ticks.pop()};
    if (tick) {
      in_order = in_order && *tick == expected;
      expected = static_cast<uint8_t>(expected + 1);
    }
  }

  TIMSK2 = saved_mask;
  TCCR2A = saved_a;
  TCCR2B = saved_b;
  OCR2A = saved_compare;

  Check(expected == kTicks && in_order,
        F("spsc_queue from an interrupt, every tick in order"));
  Check(was_full && ticks.empty(),
        F("spsc_queue full under an interrupt, no tick lost"));
}

}  // namespace self_check

ISR(TIMER2_COMPA_vect) { self_check::PushTick(); }
