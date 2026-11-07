// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Checks the library on the board, or in simavr through tools/self_check.sh,
// and prints one line per check and the number of checks that failed.
// SelfCheck.expected is what a correct run prints. The checks lie in one file
// per area, check_*.cpp; what they share in self_check.h.

#include <Utl.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "self_check.h"

void setup() {
  Serial.begin(115200);
  // The library's groups: index in words, optional as a code, so that the
  // output shows both.
  utl::UseIndexMessages();
  utl::UseCharconvMessages();

  self_check::CheckErrors();
  self_check::CheckResults();
  self_check::CheckContainers();
  self_check::CheckOptionals();
  self_check::CheckRustFluent();
  self_check::CheckReferenceWrapper();
  self_check::CheckFunctionRef();
  self_check::CheckInplaceFunction();
  self_check::CheckText();
  self_check::CheckFlashSpan();
  self_check::CheckMisuse();

  Serial.print(F("failed: "));
  Serial.println(self_check::Failed());
  Serial.flush();

  // Sleeping with interrupts off ends simavr; on a board it just stops.
  cli();
  sleep_cpu();
}

void loop() {}
