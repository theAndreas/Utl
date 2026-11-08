// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Result<T>, Result<void> and Result<T&>.

#include <Utl.h>
#include <stddef.h>
#include <stdint.h>

#include "alpha_error.h"
#include "beta_error.h"
#include "self_check.h"

namespace self_check {

using utl::Error;
using utl::Result;
using utl::Status;

namespace {

void CheckResult() {
  uint8_t calls{0};

  Check(GetCode(1).IsOk() && GetCode(1).value() == 1,
        F("Result holds a value"));
  Check(GetCode(9).status() == BetaError::kFirst &&
            *GetCode(9).error() == BetaError::kFirst,
        F("Result holds an error"));
  Check(GetCode(2).ValueOr(42) == 7 && GetCode(9).ValueOr(42) == 42,
        F("Result::ValueOr"));

  Result<bool> const mapped =
      GetCode(2).Map([](uint8_t code) { return code == 7; });
  Check(mapped.IsOk() && mapped.value(), F("Result::Map"));
  Result<bool> const not_mapped = GetCode(9).Map([&calls](uint8_t) {
    ++calls;
    return true;
  });
  Check(not_mapped.status() == BetaError::kFirst && calls == 0,
        F("Result::Map keeps the error"));

  Result<uint8_t> const chained =
      GetCode(0)
          .AndThen([](uint8_t code) { return GetCode(code + 1); })
          .AndThen([](uint8_t code) { return GetCode(code + 1); });
  Check(chained.ValueOr(0) == 7, F("Result::AndThen chains"));
  calls = 0;
  Result<uint8_t> const broken =
      GetCode(1)
          .AndThen([](uint8_t code) { return GetCode(code + 8); })
          .AndThen([&calls](uint8_t code) {
            ++calls;
            return GetCode(code);
          });
  Check(broken.status() == BetaError::kFirst && calls == 0,
        F("Result::AndThen stops at an error"));
  Status const checked = GetCode(2).AndThen([](uint8_t code) {
    return code == 7 ? Status::Ok() : Status{AlphaError::kFirst};
  });
  Check(checked.IsOk(), F("Result::AndThen into a Status"));

  Result<uint8_t> const recovered{
      GetCode(9).OrElse([](Error const&) -> uint8_t { return 3; })};
  Check(recovered.IsOk() && recovered.value() == 3, F("Result::OrElse"));
  calls = 0;
  Result<uint8_t> const kept = GetCode(2).OrElse([&calls](Error const&) {
    ++calls;
    return Result<uint8_t>{0};
  });
  Check(kept.value() == 7 && calls == 0, F("Result::OrElse keeps a value"));
}

// Stores the code at `index` in `stored`, or fails past the end: a function
// that returns no value, as a Result<void>.
Result<void> StoreCode(uint8_t index, uint8_t& stored) {
  return GetCode(index).Map([&stored](uint8_t code) { stored = code; });
}

void CheckResultVoid() {
  uint8_t stored{0};
  uint8_t calls{0};

  Result<void> const ok{Result<void>::Ok()};
  Result<void> const failed_result{AlphaError::kFirst};
  Check(ok.IsOk() && ok.error() == nullptr && Result<void>{} == ok,
        F("Result<void>::Ok() is ok"));
  Check(failed_result == AlphaError::kFirst &&
            *failed_result.error() == AlphaError::kFirst,
        F("Result<void> holds its Error"));

  Status const status{failed_result};
  Result<void> const from_status{Status{BetaError::kFirst}};
  Check(status == AlphaError::kFirst && from_status == BetaError::kFirst &&
            failed_result.status() == AlphaError::kFirst,
        F("Result<void> is a Status, and back"));

  Check(StoreCode(2, stored).IsOk() && stored == 7,
        F("Result::Map with a function that returns nothing"));
  stored = 0;
  Check(StoreCode(9, stored) == BetaError::kFirst && stored == 0,
        F("Result::Map into Result<void> keeps the error"));

  Check(Result<void>::Ok().Map([] { return uint8_t{5}; }).ValueOr(0) == 5,
        F("Result<void>::Map makes a value"));
  Result<uint8_t> const not_made{failed_result.Map([&calls] {
    ++calls;
    return uint8_t{5};
  })};
  Check(not_made.status() == AlphaError::kFirst && calls == 0,
        F("Result<void>::Map keeps the error"));
  Check(failed_result.OrElse([](Error const&) { return Status::Ok(); }).IsOk(),
        F("Result<void> recovers as a Status"));
}

// The element of `rows` at `index`, or BetaError::kFirst past the end: a
// Result of a reference.
Result<uint8_t&> GetRow(utl::array<uint8_t, 3>& rows, uint8_t index) {
  if (index >= rows.size()) {
    return BetaError::kFirst;
  }
  return rows[index];
}

void CheckResultReference() {
  utl::array<uint8_t, 3> rows{{codes[0], codes[1], codes[2]}};
  uint8_t spare{9};
  Result<uint8_t&> const found{GetRow(rows, 2)};

  Check(found.IsOk() && &found.value() == &rows[2],
        F("Result<T&> holds a reference"));
  Check(GetRow(rows, 3).status() == BetaError::kFirst,
        F("Result<T&> holds an error"));
  Result<void> const set = GetRow(rows, 1).Map([](uint8_t& row) { row = 5; });
  Check(set.IsOk() && rows[1] == 5, F("Result<T&>::Map writes through"));
  Check(&GetRow(rows, 3).ValueOr(spare) == &spare &&
            &found.ValueOr(spare) == &rows[2],
        F("Result<T&>::ValueOr gives a reference"));
  Result<uint8_t&> const chained =
      GetRow(rows, 0).AndThen([&rows](uint8_t& row) {
        return GetRow(rows, static_cast<uint8_t>(row + 2));
      });
  Check(&chained.value() == &rows[2], F("Result<T&>::AndThen chains"));
}

}  // namespace

void CheckResults() {
  CheckResult();
  CheckResultVoid();
  CheckResultReference();
}

}  // namespace self_check
