// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The functions after Rust on Result, Status and optional.

#include <Utl.h>
#include <stdint.h>

#include "alpha_error.h"
#include "beta_error.h"
#include "self_check.h"

namespace self_check {

using utl::Error;

// The functions after Rust that look on the way, end a chain, or make a
// fallback only when it is needed.
void CheckRustFluent() {
  uint8_t seen{0};
  auto const look = [&seen](uint8_t const& code) { seen = code; };
  auto const count = [&seen](Error const&) { ++seen; };

  Check(GetCode(2).Inspect(look).ValueOr(0) == 7 && seen == 7,
        F("Result::Inspect"));
  seen = 0;
  Check(
      GetCode(9).InspectError(count).status() == BetaError::kFirst && seen == 1,
      F("Result::InspectError"));
  Check(GetCode(9)
                .MapError([](Error const&) { return AlphaError::kSecond; })
                .status() == AlphaError::kSecond,
        F("Result::MapError"));
  Check(GetCode(9).ValueOrElse([](Error const&) { return uint8_t{3}; }) == 3,
        F("Result::ValueOrElse"));

  // Map() of a function that returns nothing, then ConsumeError(): both ends
  // dealt with, and nothing left that warns when dropped.
  seen = 0;
  GetCode(1).Map([&seen](uint8_t code) { seen = code; }).ConsumeError(count);
  GetCode(9).Map([&seen](uint8_t code) { seen = code; }).ConsumeError(count);
  Check(seen == 2, F("Status::ConsumeError ends a chain"));

  auto const odd = [](uint8_t const& index) { return index % 2 == 1; };
  Check(FindCode(1).Filter(odd) == uint8_t{1} &&
            FindCode(7).Filter(odd) == utl::nullopt,
        F("optional::Filter"));
  Check(FindCode(42).ValueOrElse([] { return uint8_t{5}; }) == 5,
        F("optional::ValueOrElse"));
}

}  // namespace self_check
