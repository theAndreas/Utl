// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// array, make_array() and make_array<T>().

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

TEST(ArrayTest, MakeArrayOfATypeTakesTheSizeFromTheValues) {
  constexpr auto kFont{utl::make_array<uint8_t>('F', 0, 32, 255)};
  static_assert(sizeof(kFont) == 4, "four uint8_t");
  static_assert(kFont[3] == 255, "made when compiling");
  EXPECT_EQ(kFont.size(), 4U);
  EXPECT_EQ(kFont[0], 'F');
  EXPECT_EQ(kFont[1], 0);
  EXPECT_EQ(kFont[2], 32);
  EXPECT_EQ(kFont[3], 255);
}

TEST(ArrayTest, MakeArrayOfATypeTakesNegativeValuesThatFit) {
  constexpr auto kOffsets{utl::make_array<int8_t>(-128, 0, 127)};
  static_assert(sizeof(kOffsets) == 3, "three int8_t");
  EXPECT_EQ(kOffsets[0], -128);
  EXPECT_EQ(kOffsets[1], 0);
  EXPECT_EQ(kOffsets[2], 127);
}

TEST(ArrayTest, MakeArrayOfATypeTakesElementsOfTheTypeAsTheyAre) {
  static constexpr char kFirst[] = "first";
  static constexpr char kSecond[] = "second";
  constexpr auto kTexts{utl::make_array<char const*>(kFirst, kSecond)};
  EXPECT_EQ(kTexts.size(), 2U);
  EXPECT_EQ(kTexts[0], kFirst);
  EXPECT_EQ(kTexts[1], kSecond);
}

TEST(ArrayTest, MakeArrayOfATypeTakesValuesOfTheType) {
  constexpr auto kMade{utl::make_array<uint8_t>(uint8_t{1}, uint8_t{2})};
  static_assert(sizeof(kMade) == 2, "two uint8_t");
  EXPECT_EQ(kMade[0], 1);
  EXPECT_EQ(kMade[1], 2);
}

}  // namespace
}  // namespace utl_test
