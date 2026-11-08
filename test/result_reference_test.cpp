// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Result<T&>: a reference to a T that lies elsewhere, or an Error.

#include <gtest/gtest.h>
#include <utl/array.h>
#include <utl/error.h>
#include <utl/result.h>
#include <utl/status.h>

#include <cstddef>
#include <type_traits>
#include <utility>

#include "test/alpha_error.h"
#include "test/beta_error.h"

namespace utl_test {
namespace {

using utl::Error;
using utl::Result;
using utl::Status;

struct Base {
  int id{0};
};
struct Derived : Base {};

// Returns the element of `values` at `index`, or BetaError::kFirst.
Result<int&> Get(utl::array<int, 3>& values, size_t index) {
  if (index >= values.size()) {
    return BetaError::kFirst;
  }
  return values[index];
}

// Whether ValueOr() takes an argument of type T, so that the deleted overload
// for a temporary can be checked at compile time.
template <typename T, typename = void>
struct ValueOrTakesImpl : std::false_type {};
template <typename T>
struct ValueOrTakesImpl<
    T, std::void_t<decltype(std::declval<Result<int&>>().ValueOr(
           std::declval<T>()))>> : std::true_type {};
template <typename T>
constexpr bool kValueOrTakes{ValueOrTakesImpl<T>::value};

// Never bound to a temporary, nor given one as the fallback.
static_assert(std::is_constructible_v<Result<int&>, int&>);
static_assert(!std::is_constructible_v<Result<int&>, int>);
static_assert(!std::is_constructible_v<Result<int const&>, int>);
static_assert(kValueOrTakes<int&> && !kValueOrTakes<int>);

// Converts as a pointer does, and no further.
static_assert(std::is_convertible_v<Result<int&>, Result<int const&>>);
static_assert(std::is_convertible_v<Result<Derived&>, Result<Base&>>);
static_assert(!std::is_convertible_v<Result<int const&>, Result<int&>>);
static_assert(!std::is_convertible_v<Result<Base&>, Result<Derived&>>);

TEST(ResultReferenceTest, HoldsAReference) {
  utl::array<int, 3> values{{0, 1, 7}};
  Result<int&> const found{Get(values, 2)};
  EXPECT_TRUE(found.IsOk());
  EXPECT_EQ(found.error(), nullptr);
  EXPECT_TRUE(found.status().IsOk());
  EXPECT_EQ(&found.value(), &values[2]);
  found.value() = 8;  // a const Result<int&> still gives an int&
  EXPECT_EQ(values[2], 8);
}

TEST(ResultReferenceTest, HoldsAnError) {
  utl::array<int, 3> values{{0, 1, 7}};
  Result<int&> const missing{Get(values, 3)};
  EXPECT_FALSE(missing.IsOk());
  EXPECT_EQ(missing.status(), BetaError::kFirst);
  ASSERT_NE(missing.error(), nullptr);
  EXPECT_EQ(*missing.error(), BetaError::kFirst);
  Result<int&> const from_error{Error{AlphaError::kFirst}};
  EXPECT_EQ(from_error.status(), AlphaError::kFirst);
}

TEST(ResultReferenceTest, AssigningRebinds) {
  int first{1};
  int second{2};
  Result<int&> reference{first};
  reference = second;
  EXPECT_EQ(&reference.value(), &second);
  EXPECT_EQ(first, 1);  // rebound, not written
  reference = AlphaError::kFirst;
  EXPECT_EQ(reference.status(), AlphaError::kFirst);
}

TEST(ResultReferenceTest, ConvertsAsAPointer) {
  Derived derived;
  Result<Base&> const base{Result<Derived&>{derived}};
  EXPECT_EQ(&base.value(), &derived);
  Result<Base const&> const failed{Result<Derived&>{AlphaError::kSecond}};
  EXPECT_EQ(failed.status(), AlphaError::kSecond);
}

TEST(ResultReferenceTest, ValueOrGivesAReference) {
  utl::array<int, 3> values{{0, 1, 7}};
  int fallback{9};
  EXPECT_EQ(&Get(values, 1).ValueOr(fallback), &values[1]);
  Get(values, 5).ValueOr(fallback) = 10;  // the fallback itself
  EXPECT_EQ(fallback, 10);
}

TEST(ResultReferenceTest, AndThenChainsAndStops) {
  utl::array<int, 3> values{{0, 1, 7}};
  auto const next = [&values](int& value) {
    return Get(values, static_cast<size_t>(value) + 1);
  };
  EXPECT_EQ(&Get(values, 0).AndThen(next).AndThen(next).value(), &values[2]);
  EXPECT_EQ(Get(values, 2).AndThen(next).status(), BetaError::kFirst);
  Status const checked = Get(values, 2).AndThen([](int& value) {
    return value == 7 ? Status::Ok() : AlphaError::kFirst;
  });
  EXPECT_TRUE(checked.IsOk());
}

TEST(ResultReferenceTest, AndThenCallsNothingOnAnError) {
  utl::array<int, 3> values{{0, 1, 7}};
  int calls{0};
  Result<int> const chained = Get(values, 4).AndThen([&calls](int&) {
    ++calls;
    return Result<int>{0};
  });
  EXPECT_EQ(chained.status(), BetaError::kFirst);
  EXPECT_EQ(calls, 0);
}

TEST(ResultReferenceTest, MapGetsTheReference) {
  utl::array<int, 3> values{{0, 1, 7}};
  // One that returns nothing writes through it, and gives a Result<void>.
  Result<void> const set = Get(values, 1).Map([](int& value) { value = 5; });
  EXPECT_TRUE(set.IsOk());
  EXPECT_EQ(values[1], 5);
  EXPECT_EQ(Get(values, 1).Map([](int& value) { return value * 2; }).value(),
            10);
  Result<int&> const same{
      Get(values, 1).Map([](int& value) -> int& { return value; })};
  EXPECT_EQ(&same.value(), &values[1]);
}

TEST(ResultReferenceTest, MapCallsNothingOnAnError) {
  utl::array<int, 3> values{{0, 1, 7}};
  int calls{0};
  Result<int> const mapped = Get(values, 4).Map([&calls](int&) {
    ++calls;
    return 1;
  });
  EXPECT_EQ(mapped.status(), BetaError::kFirst);
  EXPECT_EQ(calls, 0);
}

TEST(ResultReferenceTest, OrElseRecoversOrKeeps) {
  utl::array<int, 3> values{{0, 1, 7}};
  int other{9};
  auto const fallback = [&other](Error const&) -> int& { return other; };
  EXPECT_EQ(&Get(values, 4).OrElse(fallback).value(), &other);
  EXPECT_EQ(&Get(values, 0).OrElse(fallback).value(), values.data());
}

TEST(ResultReferenceTest, MapOfAValueGivesAReference) {
  // Result<T>::Map() with a function that returns a reference gives a
  // Result<T&>.
  utl::array<int, 3> values = {{0, 1, 7}};
  Result<size_t> const index{size_t{2}};
  Result<int&> const element{
      index.Map([&values](size_t at) -> int& { return values[at]; })};
  EXPECT_EQ(&element.value(), &values[2]);
}

TEST(ResultReferenceTest, ValueOrElseGivesAReference) {
  utl::array<int, 3> values{{0, 1, 7}};
  int other{9};
  auto const fallback = [&other](Error const&) -> int& { return other; };
  EXPECT_EQ(&Get(values, 1).ValueOrElse(fallback), &values[1]);
  EXPECT_EQ(&Get(values, 4).ValueOrElse(fallback), &other);
}

TEST(ResultReferenceTest, InspectPassesTheReferenceOn) {
  utl::array<int, 3> values{{0, 1, 7}};
  int seen{0};
  auto const look = [&seen](int const& value) { seen = value; };
  EXPECT_EQ(&Get(values, 2).Inspect(look).value(), &values[2]);
  EXPECT_EQ(seen, 7);
  seen = 0;
  EXPECT_EQ(Get(values, 4).Inspect(look).status(), BetaError::kFirst);
  EXPECT_EQ(seen, 0);
}

TEST(ResultReferenceTest, InspectErrorAndMapErrorPassItOn) {
  utl::array<int, 3> values{{0, 1, 7}};
  int seen{0};
  auto const count = [&seen](Error const&) { ++seen; };
  EXPECT_EQ(Get(values, 4).InspectError(count).status(), BetaError::kFirst);
  EXPECT_EQ(&Get(values, 0).InspectError(count).value(), values.data());
  EXPECT_EQ(seen, 1);
  auto const to_alpha = [](Error const&) { return AlphaError::kFirst; };
  EXPECT_EQ(Get(values, 4).MapError(to_alpha).status(), AlphaError::kFirst);
  EXPECT_EQ(&Get(values, 2).MapError(to_alpha).value(), &values[2]);
}

}  // namespace
}  // namespace utl_test
