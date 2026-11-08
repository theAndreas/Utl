// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Error and Status: what an error is, and the fluent functions of Status.

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

void CheckError() {
  Error const alpha_first{utl::ErrorGroup{self_check::kAlphaErrorGroup},
                          codes[0]};
  Error const alpha_second{utl::ErrorGroup{self_check::kAlphaErrorGroup},
                           codes[1]};
  Error const beta_first{utl::ErrorGroup{self_check::kBetaErrorGroup},
                         codes[0]};
  Error const beta_unknown{utl::ErrorGroup{self_check::kBetaErrorGroup},
                           codes[2]};

  // Until its texts are on, a group prints its code; then its text.
  PrintStatus(alpha_first);
  self_check::UseAlphaMessages();
  self_check::UseBetaMessages();

  PrintStatus(Status::Ok());
  PrintStatus(alpha_first);
  PrintStatus(alpha_second);
  PrintStatus(beta_first);
  PrintStatus(beta_unknown);
  PrintStatus(Error{utl::ErrorGroup{self_check::kBetaErrorGroup}, 10});
  PrintStatus(Error{utl::ErrorGroup{self_check::kBetaErrorGroup}, 105});

  // "alpha: alpha first", 18 chars.
  size_t const printed{alpha_first.PrintTo(Serial)};
  Serial.println();
  Check(printed == 18, F("PrintTo returns the number of chars"));

  Check(
      alpha_first.code() == 0 &&
          alpha_first.group() == utl::ErrorGroup{self_check::kAlphaErrorGroup},
      F("an Error has a group and a code"));
  Check(alpha_first == AlphaError::kFirst, F("compares with its enum"));
  Check(AlphaError::kFirst == alpha_first, F("with the enum on the left"));
  Check(alpha_first != AlphaError::kSecond, F("differs in the code"));
  Check(beta_first != AlphaError::kFirst, F("differs in the group"));
  Check(alpha_first != beta_first, F("same code, other group"));

  Error const braced{BetaError::kFirst};
  // = rather than braces: what is checked is that a code converts implicitly.
  Error const converted = BetaError::kFirst;
  Check(braced == beta_first && converted == beta_first,
        F("made from its enum"));
  Check(self_check::MakeAlphaSecondElsewhere() == alpha_second,
        F("equal across translation units"));

  Error assigned{alpha_first};
  assigned = beta_first;
  Check(assigned == BetaError::kFirst, F("assignable"));
}

void CheckStatus() {
  Status const ok{Status::Ok()};
  Status const alpha_first{AlphaError::kFirst};

  Check(ok.IsOk() && ok.error() == nullptr && ok == Status{},
        F("Status::Ok() is ok and has no Error"));
  Check(!alpha_first.IsOk() && alpha_first.error() != nullptr &&
            *alpha_first.error() == AlphaError::kFirst,
        F("a Status holds its Error"));
  Check(alpha_first == AlphaError::kFirst && alpha_first != BetaError::kFirst &&
            alpha_first != ok,
        F("Status compares with a code"));
  Check((alpha_first == Error{AlphaError::kFirst}) &&
            (Error{AlphaError::kFirst}) == alpha_first,
        F("Status compares with an Error"));
}

void CheckStatusFluent() {
  Status const alpha_first{AlphaError::kFirst};
  uint8_t calls{0};

  Check(Status::Ok().AndThen([] { return Status{AlphaError::kSecond}; }) ==
            AlphaError::kSecond,
        F("Status::AndThen goes on when ok"));
  Status const stopped{alpha_first.AndThen([&calls] {
    ++calls;
    return Status::Ok();
  })};
  Check(stopped == AlphaError::kFirst && calls == 0,
        F("Status::AndThen stops at an error"));
  Check(Status::Ok().AndThen([] { return Result<uint8_t>{5}; }).ValueOr(0) == 5,
        F("Status::AndThen into a Result"));

  Check(alpha_first.OrElse([](Error const&) { return Status::Ok(); }).IsOk(),
        F("Status::OrElse recovers"));
  Check(alpha_first.OrElse([](Error const& error) {
    return error == AlphaError::kFirst ? Status{BetaError::kFirst}
                                       : Status::Ok();
  }) == BetaError::kFirst,
        F("Status::OrElse gets the Error"));
  calls = 0;
  Status const untouched = Status::Ok().OrElse([&calls](Error const&) {
    ++calls;
    return Status{AlphaError::kFirst};
  });
  Check(untouched.IsOk() && calls == 0, F("Status::OrElse leaves ok alone"));
}

}  // namespace

void CheckErrors() {
  CheckError();
  CheckStatus();
  CheckStatusFluent();
}

}  // namespace self_check
