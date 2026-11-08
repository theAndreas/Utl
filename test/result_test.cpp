// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Result<T> and Result<void>: a value or an Error, and the fluent functions.

#include <gtest/gtest.h>
#include <utl/error.h>
#include <utl/result.h>
#include <utl/status.h>

#include "test/alpha_error.h"
#include "test/beta_error.h"

namespace utl_test {
namespace {

using utl::Error;
using utl::Result;
using utl::Status;

// Returns `code` for an index below 3, or BetaError::kFirst.
Result<int> Get(int index) {
  if (index >= 3) {
    return BetaError::kFirst;
  }
  return index * 10;
}

TEST(ResultTest, HoldsAValue) {
  Result<int> const result{Get(2)};
  EXPECT_TRUE(result.IsOk());
  EXPECT_EQ(result.value(), 20);
  EXPECT_EQ(result.error(), nullptr);
  EXPECT_TRUE(result.status().IsOk());
}

TEST(ResultTest, HoldsAnError) {
  Result<int> const result{Get(3)};
  EXPECT_FALSE(result.IsOk());
  ASSERT_NE(result.error(), nullptr);
  EXPECT_EQ(*result.error(), BetaError::kFirst);
  EXPECT_EQ(result.status(), BetaError::kFirst);
  EXPECT_EQ(Result<int>{Error{AlphaError::kFirst}}.status(),
            AlphaError::kFirst);
}

TEST(ResultTest, ValueOr) {
  EXPECT_EQ(Get(1).ValueOr(-1), 10);
  EXPECT_EQ(Get(5).ValueOr(-1), -1);
}

TEST(ResultTest, MapChangesTheValue) {
  EXPECT_EQ(Get(2).Map([](int value) { return value + 1; }).ValueOr(0), 21);
  int calls{0};
  Result<bool> const mapped = Get(5).Map([&calls](int) {
    ++calls;
    return true;
  });
  EXPECT_EQ(mapped.status(), BetaError::kFirst);
  EXPECT_EQ(calls, 0);
}

TEST(ResultTest, AndThenChainsAndStops) {
  EXPECT_EQ(Get(0).AndThen([](int) { return Get(2); }).ValueOr(0), 20);
  int calls{0};
  Result<int> const broken =
      Get(1).AndThen([](int) { return Get(9); }).AndThen([&calls](int value) {
        ++calls;
        return Get(value);
      });
  EXPECT_EQ(broken.status(), BetaError::kFirst);
  EXPECT_EQ(calls, 0);
  Status const checked = Get(2).AndThen([](int value) {
    return value == 20 ? Status::Ok() : Status{AlphaError::kFirst};
  });
  EXPECT_TRUE(checked.IsOk());
}

TEST(ResultTest, OrElseRecoversOrKeeps) {
  EXPECT_EQ(Get(9).OrElse([](Error const&) { return 7; }).ValueOr(0), 7);
  int calls{0};
  Result<int> const kept = Get(1).OrElse([&calls](Error const&) {
    ++calls;
    return Result<int>{0};
  });
  EXPECT_EQ(kept.value(), 10);
  EXPECT_EQ(calls, 0);
}

TEST(ResultVoidTest, IsAStatus) {
  Result<void> const ok{Result<void>::Ok()};
  EXPECT_TRUE(ok.IsOk());
  EXPECT_EQ(ok, Result<void>{});
  Result<void> const failed{AlphaError::kFirst};
  Status const status{failed};
  EXPECT_EQ(status, AlphaError::kFirst);
  EXPECT_EQ(failed.status(), AlphaError::kFirst);
  Result<void> const from_status{Status{BetaError::kFirst}};
  EXPECT_EQ(from_status, BetaError::kFirst);
}

TEST(ResultVoidTest, MapOfAFunctionThatReturnsNothing) {
  int stored{0};
  Result<void> const done =
      Get(2).Map([&stored](int value) { stored = value; });
  EXPECT_TRUE(done.IsOk());
  EXPECT_EQ(stored, 20);
  stored = 0;
  Result<void> const not_done =
      Get(9).Map([&stored](int value) { stored = value; });
  EXPECT_EQ(not_done, BetaError::kFirst);
  EXPECT_EQ(stored, 0);
}

TEST(ResultVoidTest, MapMakesAValue) {
  EXPECT_EQ(Result<void>::Ok().Map([] { return 5; }).ValueOr(0), 5);
  int calls{0};
  Result<int> const none{Result<void>{AlphaError::kFirst}.Map([&calls] {
    ++calls;
    return 5;
  })};
  EXPECT_EQ(none.status(), AlphaError::kFirst);
  EXPECT_EQ(calls, 0);
  int printed{0};
  Result<void> const nothing{Result<void>::Ok().Map([&printed] { ++printed; })};
  EXPECT_TRUE(nothing.IsOk());
  EXPECT_EQ(printed, 1);
}

TEST(ResultTest, ValueOrElseMakesTheFallbackFromTheError) {
  int calls{0};
  auto const fallback = [&calls](Error const& error) {
    ++calls;
    return error == BetaError::kFirst ? -1 : -2;
  };
  EXPECT_EQ(Get(1).ValueOrElse(fallback), 10);
  EXPECT_EQ(calls, 0);
  EXPECT_EQ(Get(5).ValueOrElse(fallback), -1);
  EXPECT_EQ(calls, 1);
}

TEST(ResultTest, InspectLooksAtTheValue) {
  int seen{0};
  auto const look = [&seen](int const& value) { seen = value; };
  EXPECT_EQ(Get(2).Inspect(look).value(), 20);
  EXPECT_EQ(seen, 20);
  seen = 0;
  EXPECT_EQ(Get(5).Inspect(look).status(), BetaError::kFirst);
  EXPECT_EQ(seen, 0);
}

TEST(ResultTest, InspectErrorLooksAtTheError) {
  int seen{0};
  auto const look = [&seen](Error const&) { ++seen; };
  EXPECT_EQ(Get(5).InspectError(look).status(), BetaError::kFirst);
  EXPECT_EQ(Get(2).InspectError(look).value(), 20);
  EXPECT_EQ(seen, 1);
}

TEST(ResultTest, MapErrorKeepsTheValue) {
  auto const to_alpha = [](Error const&) { return AlphaError::kSecond; };
  EXPECT_EQ(Get(5).MapError(to_alpha).status(), AlphaError::kSecond);
  EXPECT_EQ(Get(1).MapError(to_alpha).value(), 10);
}

}  // namespace
}  // namespace utl_test
