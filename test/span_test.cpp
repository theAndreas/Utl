// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// span: a view of elements that lie one after the other.

#include <gtest/gtest.h>
#include <utl/array.h>
#include <utl/index_error.h>
#include <utl/span.h>

#include <cstddef>

namespace utl_test {
namespace {

using utl::IndexError;
using utl::span;

int Sum(span<int const> values) {
  int sum{0};
  for (int const value : values) {
    sum += value;
  }
  return sum;
}

TEST(SpanTest, MadeFromAnything) {
  utl::array<int, 4> rows{{0, 1, 7, 3}};
  utl::array<int, 4> const& const_rows{rows};
  // A C array, as a span is to take one.
  int c_array[]{7, 7};  // NOLINT(modernize-avoid-c-arrays)
  span<int> const all{rows};
  EXPECT_TRUE(span<int>{}.empty());
  EXPECT_EQ(span<int>{}.data(), nullptr);
  EXPECT_EQ(all.size(), 4U);
  EXPECT_EQ(all.size_bytes(), 4 * sizeof(int));
  EXPECT_EQ(Sum(rows), 11);
  EXPECT_EQ(Sum(const_rows), 11);
  EXPECT_EQ(Sum(c_array), 14);
  EXPECT_EQ(Sum(all), 11);
  EXPECT_EQ(Sum(span<int const>{rows.data(), 2}), 1);
}

TEST(SpanTest, WritesThrough) {
  utl::array<int, 4> rows{{0, 1, 7, 3}};
  span<int> const all{rows};
  all[0] = 9;
  EXPECT_EQ(rows[0], 9);
  EXPECT_EQ(all.front(), 9);
  EXPECT_EQ(all.back(), 3);
  EXPECT_EQ(all.end() - all.begin(), 4);
}

TEST(SpanTest, CheckedAccess) {
  utl::array<int, 4> rows{{0, 1, 7, 3}};
  span<int> const all{rows};
  EXPECT_EQ(all.at(3).ValueOr(0), 3);
  EXPECT_EQ(all.at(4).status(), IndexError::kOutOfRange);
  EXPECT_EQ(Sum(all.first(2).value()), 1);
  EXPECT_EQ(Sum(all.last(1).value()), 3);
  EXPECT_EQ(Sum(all.subspan(1, 2).value()), 8);
  EXPECT_EQ(Sum(all.subspan(1).value()), 11);
  EXPECT_TRUE(all.subspan(4).value().empty());
}

TEST(SpanTest, BeyondTheEnd) {
  utl::array<int, 4> rows{{0, 1, 7, 3}};
  span<int> const all{rows};
  EXPECT_EQ(all.first(5).status(), IndexError::kOutOfRange);
  EXPECT_EQ(all.last(5).status(), IndexError::kOutOfRange);
  EXPECT_EQ(all.subspan(5).status(), IndexError::kOutOfRange);
  EXPECT_EQ(all.subspan(5, 0).status(), IndexError::kOutOfRange);
  EXPECT_EQ(all.subspan(2, 3).status(), IndexError::kOutOfRange);
  // offset + count would overflow; the check must not.
  EXPECT_EQ(all.subspan(1, static_cast<size_t>(-1)).status(),
            IndexError::kOutOfRange);
}

}  // namespace
}  // namespace utl_test
