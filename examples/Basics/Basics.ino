// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Two groups that the library does not know, errors from both, and how a
// sketch checks, chains and prints them. The bus says what went wrong in
// words; the font gives its code, as its texts are not turned on and not
// linked.

#include <Utl.h>

#include "bus_error.h"
#include "font_error.h"

using bus::BusError;
using font::FontError;
using utl::Error;
using utl::Result;
using utl::Status;

namespace {

// Fails on a char the font lacks. Takes its input from a pin in loop(), so
// that the compiler cannot fold the error away.
Status SetChar(char c) {
  if (c < ' ') {
    return FontError::kUnknownChar;
  }
  return Status::Ok();
}

// Returns the char the other side sent back, or the error of the bus.
Result<char> Receive(bool acknowledged) {
  if (!acknowledged) {
    return BusError::kNack;
  }
  return 'A';
}

}  // namespace

void setup() {
  Serial.begin(115200);
  bus::UseBusMessages();
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
}

void loop() {
  // Receive a char and show it; either step may fail, and the first error
  // ends the chain.
  Status const status{
      Receive(digitalRead(3) == HIGH).AndThen([](char received) {
        return SetChar((digitalRead(2) == LOW) ? '\n' : received);
      })};
  status.PrintTo(Serial);  // "bus: nack", "font: 0" or "ok"
  Serial.println();

  // Both codes are 0, in different groups: only the group tells them apart.
  if (status == FontError::kUnknownChar) {
    Serial.println(F("pick another char"));
  }
  if (status == BusError::kNack) {
    Serial.println(F("retry"));
  }

  Error const unknown_char{FontError::kUnknownChar};
  if (SetChar('\n') == unknown_char) {
    Serial.println(F("a control char is not in the font"));
  }
  delay(1000);
}
