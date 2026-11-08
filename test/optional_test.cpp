// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// optional: a value or none.

#include <gtest/gtest.h>
#include <utl/array.h>
#include <utl/optional.h>
#include <utl/optional_error.h>

#include "test/alpha_error.h"

namespace utl_test {
namespace {

using utl::nullopt;
using utl::optional;

// Returns the index of `value` in {0, 1, 7}, or none.
optional<int> Find(int value) {
  utl::array<int, 3> const values{{0, 1, 7}};
  for (int index{0}; index < 3; ++index) {
    if (values[index] == value) {
      return index;
    }
  }
  return nullopt;
}

TEST(OptionalTest, EmptyOrAValue) {
  optional<int> const none{Find(42)};
  optional<int> const found{Find(7)};
  EXPECT_FALSE(none.HasValue());
  EXPECT_FALSE(static_cast<bool>(none));
  EXPECT_EQ(none, nullopt);
  EXPECT_EQ(optional<int>{}, none);
  EXPECT_TRUE(found.HasValue());
  EXPECT_EQ(*found, 2);
  EXPECT_EQ(found, 2);
  EXPECT_NE(found, none);
  EXPECT_NE(found, 3);
}

TEST(OptionalTest, ArrowReachesTheValue) {
  struct Pair {
    int first;
    int second;
    bool operator==(Pair const& other) const {
      return first == other.first && second == other.second;
    }
  };
  optional<Pair> pair{Pair{1, 2}};
  EXPECT_EQ(pair->second, 2);
  pair->second = 3;
  optional<Pair> const& same{pair};
  EXPECT_EQ(same->second, 3);
  *pair = Pair{4, 5};
  EXPECT_EQ((*same).first, 4);
}

TEST(OptionalTest, ValueReturnsAResult) {
  EXPECT_EQ(Find(7).value().ValueOr(0), 2);
  EXPECT_EQ(Find(42).value().status(), utl::OptionalError::kEmpty);
  EXPECT_EQ(Find(7).ValueOr(9), 2);
  EXPECT_EQ(Find(42).ValueOr(9), 9);
}

TEST(OptionalTest, OkOrGivesYourError) {
  EXPECT_EQ(Find(42).OkOr(AlphaError::kSecond).status(), AlphaError::kSecond);
  EXPECT_EQ(Find(7).OkOr(AlphaError::kSecond).ValueOr(0), 2);
}

TEST(OptionalTest, ResetEmplaceAndAssign) {
  // = rather than braces: what is checked is that a value converts implicitly.
  optional<int> changed = 2;
  changed.reset();
  EXPECT_FALSE(changed.HasValue());
  EXPECT_EQ(changed.emplace(5), 5);
  EXPECT_EQ(changed, 5);
  changed = nullopt;
  EXPECT_EQ(changed, nullopt);
  changed = 6;
  changed = {};
  EXPECT_FALSE(changed.HasValue());
}

TEST(OptionalTest, AndThen) {
  auto const find_again = [](int index) { return Find(index); };
  EXPECT_EQ(Find(1).AndThen(find_again), 1);
  EXPECT_EQ(Find(7).AndThen(find_again), nullopt);
  EXPECT_EQ(Find(42).AndThen(find_again), nullopt);
}

TEST(OptionalTest, Map) {
  EXPECT_EQ(Find(7).Map([](int index) { return index * 10; }), 20);
  int calls{0};
  EXPECT_EQ(Find(42).Map([&calls](int) {
    ++calls;
    return 1;
  }),
            nullopt);
  EXPECT_EQ(calls, 0);
}

TEST(OptionalTest, OrElse) {
  int calls{0};
  auto const three = [&calls] {
    ++calls;
    return optional<int>{3};
  };
  EXPECT_EQ(Find(42).OrElse(three), 3);
  EXPECT_EQ(Find(7).OrElse(three), 2);
  EXPECT_EQ(calls, 1);
}

TEST(OptionalTest, ValueOrElseCallsOnlyWhenEmpty) {
  int calls{0};
  auto const fallback = [&calls] {
    ++calls;
    return 9;
  };
  EXPECT_EQ(Find(7).ValueOrElse(fallback), 2);
  EXPECT_EQ(calls, 0);
  EXPECT_EQ(Find(42).ValueOrElse(fallback), 9);
  EXPECT_EQ(calls, 1);
}

TEST(OptionalTest, InspectLooksAtTheValue) {
  int seen{-1};
  auto const look = [&seen](int const& index) { seen = index; };
  EXPECT_EQ(Find(7).Inspect(look), 2);
  EXPECT_EQ(seen, 2);
  seen = -1;
  EXPECT_EQ(Find(42).Inspect(look), nullopt);
  EXPECT_EQ(seen, -1);
}

TEST(OptionalTest, FilterKeepsWhatHolds) {
  auto const even = [](int const& index) { return index % 2 == 0; };
  EXPECT_EQ(Find(7).Filter(even), 2);
  EXPECT_EQ(Find(1).Filter(even), nullopt);
  EXPECT_EQ(Find(42).Filter(even), nullopt);
}

TEST(OptionalTest, AndThenMayChangeTheType) {
  auto const letter = [](int index) {
    return optional<char>{static_cast<char>('a' + index)};
  };
  EXPECT_EQ(Find(7).AndThen(letter), 'c');
}

}  // namespace
}  // namespace utl_test
