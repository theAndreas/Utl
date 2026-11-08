// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// function_ref, as a parameter above all: the pointer to a function on the
// AVR is a word address, which has to come back as it went in. A member
// function given with UTL_CW() is called on its object, through a reference
// or a pointer.

#include <Utl.h>
#include <stdint.h>

#include "self_check.h"

namespace self_check {
namespace {

uint8_t Doubled(uint8_t value) { return static_cast<uint8_t>(2 * value); }
uint8_t Zero(uint8_t /*value*/) { return 0; }

// Adds what f gives for each code: a function_ref as a parameter, what it is
// made for.
uint8_t SumOfCodes(utl::function_ref<uint8_t(uint8_t)> f) {
  uint8_t sum{0};
  for (uint8_t const code : codes) {
    sum = static_cast<uint8_t>(sum + f(code));
  }
  return sum;
}

// A member function to bind, and one of const.
class Tally {
 public:
  void Add(uint8_t code) { sum_ = static_cast<uint8_t>(sum_ + code); }
  uint8_t Sum() const { return sum_; }

 private:
  uint8_t sum_{0};
};

// Calls f once per code: a member function bound to its object.
void ForEachCode(utl::function_ref<void(uint8_t)> f) {
  for (uint8_t const code : codes) {
    f(code);
  }
}

}  // namespace

void CheckFunctionRef() {
  // The codes are 0, 1 and 7.
  Check(SumOfCodes(Doubled) == 16, F("function_ref calls a function"));

  uint8_t offset{codes[1]};
  auto const add = [&offset](uint8_t code) {
    return static_cast<uint8_t>(code + offset);
  };
  Check(SumOfCodes(add) == 11, F("function_ref calls a lambda"));
  offset = codes[2];
  utl::function_ref<uint8_t(uint8_t)> f{add};
  Check(f(0) == 7, F("function_ref refers to the lambda, not a copy"));

  f = Zero;
  utl::function_ref<uint16_t(uint8_t)> const widened{Doubled};
  Check(f(codes[2]) == 0 && widened(codes[2]) == 14,
        F("function_ref rebinds, and to a function of other types"));
  Check(SumOfCodes([](uint8_t code) { return code; }) == 8,
        F("function_ref of a lambda written as the argument"));

  Tally tally;
  ForEachCode({UTL_CW(&Tally::Add), tally});
  utl::function_ref<uint8_t()> const sum{UTL_CW(&Tally::Sum), &tally};
  Check(sum() == 8 && tally.Sum() == 8,
        F("function_ref calls a member function of its object"));
  utl::function_ref<uint16_t(Tally const&)> const sum_of{UTL_CW(&Tally::Sum)};
  utl::function_ref<uint8_t(uint8_t)> const doubled{UTL_CW(Doubled)};
  Check(sum_of(tally) == 8 && doubled(codes[2]) == 14,
        F("function_ref calls a function given as a constant"));
}

}  // namespace self_check
