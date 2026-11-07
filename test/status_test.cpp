// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Status: ok or an Error, and its fluent functions.

#include <gtest/gtest.h>
#include <utl/error.h>
#include <utl/result.h>
#include <utl/status.h>

#include <cstddef>
#include <string>

#include "test/alpha_error.h"
#include "test/beta_error.h"
#include "test/string_print.h"

namespace utl_test {
namespace {

using utl::Error;
using utl::Result;
using utl::Status;

TEST(StatusTest, OkHasNoError) {
  Status const ok{Status::Ok()};
  EXPECT_TRUE(ok.IsOk());
  EXPECT_EQ(ok.error(), nullptr);
  EXPECT_EQ(ok, Status{});
  EXPECT_STREQ(reinterpret_cast<char const*>(ok.Message()), "ok");
}

TEST(StatusTest, HoldsItsError) {
  Status const status{AlphaError::kSecond};
  EXPECT_FALSE(status.IsOk());
  ASSERT_NE(status.error(), nullptr);
  EXPECT_EQ(*status.error(), AlphaError::kSecond);
  EXPECT_EQ(status, AlphaError::kSecond);
  EXPECT_EQ(status, Error{AlphaError::kSecond});
  EXPECT_EQ(Error{AlphaError::kSecond}, status);
  EXPECT_NE(status, BetaError::kFirst);
  EXPECT_NE(status, Status::Ok());
}

// The text of a group whose texts are on, and none of one whose are not.
TEST(StatusTest, MessageOfAnError) {
  UseAlphaMessages();
  EXPECT_STREQ(
      reinterpret_cast<char const*>(Status{AlphaError::kFirst}.Message()),
      "alpha first");
  EXPECT_STREQ(
      reinterpret_cast<char const*>(Status{BetaError::kFirst}.Message()), "");
}

Status SetAlpha() { return AlphaError::kFirst; }
Result<void> SetBeta() { return BetaError::kFirst; }

// A Status may be dropped without a warning, as a sketch drops the status of
// SetText(); the tests are built with -Werror, so a [[nodiscard]] back on the
// type fails this file.
TEST(StatusTest, MayBeDropped) {
  SetAlpha();
  SetBeta();
}

TEST(StatusTest, PrintsOkOrTheError) {
  StringPrint ok;
  EXPECT_EQ(Status::Ok().PrintTo(ok), 2U);
  EXPECT_EQ(ok.text, "ok");
  UseAlphaMessages();
  StringPrint alpha;
  size_t const count{Status{AlphaError::kFirst}.PrintTo(alpha)};
  EXPECT_EQ(count, alpha.text.size());
  EXPECT_EQ(alpha.text, "alpha: alpha first");
  StringPrint beta;
  static_cast<void>(Status{BetaError::kFirst}.PrintTo(beta));
  EXPECT_EQ(beta.text, "beta: 0");
}

TEST(StatusTest, AndThenGoesOnWhenOk) {
  EXPECT_EQ(Status::Ok().AndThen([] { return Status{AlphaError::kSecond}; }),
            AlphaError::kSecond);
  EXPECT_EQ(Status::Ok().AndThen([] { return Result<int>{5}; }).ValueOr(0), 5);
}

TEST(StatusTest, AndThenStopsAtAnError) {
  int calls{0};
  Status const stopped{Status{AlphaError::kFirst}.AndThen([&calls] {
    ++calls;
    return Status::Ok();
  })};
  EXPECT_EQ(stopped, AlphaError::kFirst);
  EXPECT_EQ(calls, 0);
  Result<int> const no_value{
      Status{AlphaError::kFirst}.AndThen([] { return Result<int>{5}; })};
  EXPECT_EQ(no_value.status(), AlphaError::kFirst);
}

TEST(StatusTest, OrElseGetsTheErrorAndRecovers) {
  Status const recovered =
      Status(AlphaError::kFirst).OrElse([](Error const& error) {
        return error == AlphaError::kFirst ? Status::Ok()
                                           : Status{BetaError::kFirst};
      });
  EXPECT_TRUE(recovered.IsOk());
  int calls{0};
  Status const untouched = Status::Ok().OrElse([&calls](Error const&) {
    ++calls;
    return Status{AlphaError::kFirst};
  });
  EXPECT_TRUE(untouched.IsOk());
  EXPECT_EQ(calls, 0);
}

TEST(StatusTest, InspectErrorLooksAndPassesOn) {
  int seen{0};
  auto const count = [&seen](Error const& error) {
    seen = error == AlphaError::kFirst ? 1 : 2;
  };
  Status const failed{AlphaError::kFirst};
  EXPECT_EQ(failed.InspectError(count), AlphaError::kFirst);
  EXPECT_EQ(seen, 1);
  seen = 0;
  EXPECT_TRUE(Status::Ok().InspectError(count).IsOk());
  EXPECT_EQ(seen, 0);
}

TEST(StatusTest, MapErrorSaysItInAnotherGroup) {
  auto const to_beta = [](Error const&) { return BetaError::kFirst; };
  Status const failed{AlphaError::kFirst};
  EXPECT_EQ(failed.MapError(to_beta), BetaError::kFirst);
  EXPECT_TRUE(Status::Ok().MapError(to_beta).IsOk());
  // An Error works as well as a code.
  EXPECT_EQ(
      failed.MapError([](Error const&) { return Error{BetaError::kFirst}; }),
      BetaError::kFirst);
}

TEST(StatusTest, ConsumeErrorEndsAChain) {
  int seen{0};
  auto const count = [&seen](Error const&) { ++seen; };
  Status const failed{AlphaError::kFirst};
  failed.ConsumeError(count);
  Status::Ok().ConsumeError(count);
  EXPECT_EQ(seen, 1);
  // A Result<void> is a Status: Map() of a function that returns nothing,
  // then ConsumeError(), deals with both ends, and nothing is left to drop.
  Result<int> const value = 7;
  value.Map([&seen](int v) { seen += v; }).ConsumeError(count);
  EXPECT_EQ(seen, 8);
}

// What the checks on what `f` returns let pass, at their edges; the cases
// they stop lie in test/compile_fail/.
TEST(StatusTest, FluentFunctionsTakeWhatTheyShould) {
  Status const failed{AlphaError::kFirst};
  // MapError(): an Error by reference is an Error.
  Error const beta{BetaError::kFirst};
  EXPECT_EQ(
      failed.MapError([&beta](Error const&) -> Error const& { return beta; }),
      BetaError::kFirst);
  // AndThen(): a Result<void> is a Result, a const Result one as well.
  EXPECT_TRUE(Status::Ok().AndThen([] { return Result<void>::Ok(); }).IsOk());
  EXPECT_EQ(
      Status::Ok().AndThen([]() -> Result<int> const { return 2; }).ValueOr(0),
      2);
}

}  // namespace
}  // namespace utl_test
