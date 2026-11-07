// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The report of each check, and the codes and lookups the areas share.

#include "self_check.h"

#include <Utl.h>
#include <stddef.h>
#include <stdint.h>

#include "beta_error.h"

namespace self_check {

utl::array<uint8_t volatile, 3> codes{{0, 1, 7}};

namespace {

uint8_t failed{0};

}  // namespace

void Check(bool passed, __FlashStringHelper const* name) {
  Serial.print(passed ? F("pass ") : F("FAIL "));
  Serial.println(name);
  if (!passed) {
    ++failed;
  }
}

uint8_t Failed() { return failed; }

void PrintStatus(utl::Status const& status) {
  status.PrintTo(Serial);
  Serial.println();
}

utl::Result<uint8_t> GetCode(uint8_t index) {
  if (index >= codes.size()) {
    return BetaError::kFirst;
  }
  // Read first: a const reference does not bind to a volatile.
  uint8_t const code{codes[index]};
  return code;
}

utl::optional<uint8_t> FindCode(uint8_t wanted) {
  for (size_t index{0}; index < codes.size(); ++index) {
    if (codes[index] == wanted) {
      return static_cast<uint8_t>(index);  // codes has 3 elements
    }
  }
  return utl::nullopt;
}

}  // namespace self_check
