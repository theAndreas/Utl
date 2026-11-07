// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// array and make_array.

#include <gtest/gtest.h>
#include <utl/array.h>
#include <utl/index_error.h>

#include <cstdint>

namespace utl_test {
namespace {

using utl::array;

TEST(ArrayTest, SizeAndElements) {
  array<int, 3> rows{{1, 2, 3}};
  EXPECT_EQ(rows.size(), 3U);
  EXPECT_EQ(rows.max_size(), 3U);
  EXPECT_FALSE(rows.empty());
  EXPECT_EQ(rows[1], 2);
  EXPECT_EQ(rows.front(), 1);
  EXPECT_EQ(rows.back(), 3);
  EXPECT_EQ(rows.data(), &rows.front());
  rows.front() = 4;
  rows.back() = 6;
  rows[1] = 5;
  array<int, 3> const& same{rows};
  EXPECT_EQ(same.front() + same[1] + same.back(), 15);
  EXPECT_EQ(same.data(), rows.data());
}

TEST(ArrayTest, Iterates) {
  array<int, 3> rows{{1, 2, 3}};
  int sum{0};
  for (int& row : rows) {
    row *= 2;
  }
  for (int const row : static_cast<array<int, 3> const&>(rows)) {
    sum += row;
  }
  EXPECT_EQ(sum, 12);
  EXPECT_EQ(rows.cend() - rows.cbegin(), 3);
}

TEST(ArrayTest, AtReturnsAResult) {
  array<int, 3> const rows{{1, 2, 3}};
  EXPECT_EQ(rows.at(2).ValueOr(0), 3);
  EXPECT_EQ(rows.at(3).status(), utl::IndexError::kOutOfRange);
}

TEST(ArrayTest, CopiesComparesAndFills) {
  array<int, 3> const rows{{1, 2, 3}};
  array<int, 3> copy{rows};
  EXPECT_EQ(copy, rows);
  copy.fill(5);
  EXPECT_NE(copy, rows);
  EXPECT_EQ(copy[0] + copy[2], 10);
}

TEST(ArrayTest, MakeArrayTakesTypeAndSize) {
  constexpr auto kMade = utl::make_array(uint8_t{1}, uint8_t{2});
  static_assert(sizeof(kMade) == 2, "two uint8_t");
  EXPECT_EQ(kMade.size(), 2U);
  EXPECT_EQ(kMade[1], 2);
}

}  // namespace
}  // namespace utl_test
