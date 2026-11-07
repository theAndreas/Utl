// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// flash_span: a view of elements in flash. On the host flash is RAM, so these
// tests see what it gives, not that it reads flash; SelfCheck in simavr does.

#include <avr/pgmspace.h>
#include <gtest/gtest.h>
#include <utl/array.h>
#include <utl/flash_span.h>
#include <utl/index_error.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace utl_test {
namespace {

using utl::flash_span;
using utl::IndexError;

// Elements of each width that is read apart: 1, 2 and 4 bytes, and 3, which
// memcpy_P() reads; one without a default constructor.
struct Color {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct Pin {
  explicit constexpr Pin(uint8_t number_in) : number(number_in) {}
  uint8_t number;
};

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr uint8_t kRows[] PROGMEM = {0x3c, 0x42, 0xa5, 0x81};
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr uint16_t kSteps[] PROGMEM = {100, 200, 400};
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr int32_t kOffsets[] PROGMEM = {-70000, 70000};
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr Color kColors[] PROGMEM = {{255, 0, 0}, {0, 128, 255}};
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr Pin kPins[] PROGMEM = {Pin(2), Pin(13)};
constexpr utl::array<uint8_t, 3> kArray PROGMEM = {{7, 8, 9}};

static_assert(sizeof(flash_span<uint8_t>) == sizeof(void*) + sizeof(size_t));
static_assert(std::is_trivially_copyable_v<flash_span<Color>>);
// Made of an array only explicitly, as its type does not tell where it lies.
using Rows = uint8_t const (&)[4];  // NOLINT(modernize-avoid-c-arrays)
static_assert(!std::is_convertible_v<Rows, flash_span<uint8_t>>);
static_assert(std::is_constructible_v<flash_span<uint8_t>, Rows>);
static_assert(
    !std::is_convertible_v<utl::array<uint8_t, 3> const&, flash_span<uint8_t>>);
// Of the elements' own type only.
static_assert(!std::is_constructible_v<flash_span<uint16_t>, Rows>);

constexpr flash_span<uint8_t> kRowsSpan(kRows);
static_assert(kRowsSpan.size() == 4 && kRowsSpan.size_bytes() == 4);
static_assert(flash_span<uint16_t>(kSteps).size_bytes() == 6);

template <typename T>
std::vector<T> Read(flash_span<T> elements) {
  std::vector<T> read;
  for (T const element : elements) {
    read.push_back(element);
  }
  return read;
}

TEST(FlashSpanTest, ReadsEachWidth) {
  flash_span<uint8_t> const rows(kRows);
  flash_span<uint16_t> const steps(kSteps);
  flash_span<int32_t> const offsets(kOffsets);
  flash_span<Color> const colors(kColors);
  EXPECT_EQ(rows[2], 0xa5);
  EXPECT_EQ(steps[1], 200);
  EXPECT_EQ(offsets[0], -70000);
  EXPECT_EQ(colors[1].blue, 255);
  EXPECT_EQ(flash_span<Pin>(kPins)[1].number, 13);
}

TEST(FlashSpanTest, GoesThroughARangeBasedFor) {
  EXPECT_EQ(Read(flash_span<uint8_t>(kRows)),
            (std::vector<uint8_t>{0x3c, 0x42, 0xa5, 0x81}));
  EXPECT_EQ(Read(flash_span<uint16_t>(kSteps)),
            (std::vector<uint16_t>{100, 200, 400}));
  EXPECT_TRUE(Read(flash_span<uint8_t>()).empty());
}

TEST(FlashSpanTest, IsMadeOfAnArrayOrAPointerAndASize) {
  flash_span<uint8_t> const of_array(kArray);
  EXPECT_EQ(Read(of_array), (std::vector<uint8_t>{7, 8, 9}));
  flash_span<uint8_t> const of_pointer(kRows + 1, 2);
  EXPECT_EQ(Read(of_pointer), (std::vector<uint8_t>{0x42, 0xa5}));
  EXPECT_EQ(of_pointer.data(), kRows + 1);
  flash_span<uint8_t> const none;
  EXPECT_TRUE(none.empty());
  EXPECT_EQ(none.data(), nullptr);
}

TEST(FlashSpanTest, GivesItsElements) {
  flash_span<uint16_t> const steps(kSteps);
  EXPECT_FALSE(steps.empty());
  EXPECT_EQ(steps.front(), 100);
  EXPECT_EQ(steps.back(), 400);
  EXPECT_EQ(steps.at(2).value(), 400);
  EXPECT_EQ(steps.at(3).status(), IndexError::kOutOfRange);
}

TEST(FlashSpanTest, TakesAPart) {
  flash_span<uint8_t> const rows(kRows);
  EXPECT_EQ(Read(rows.first(2).value()), (std::vector<uint8_t>{0x3c, 0x42}));
  EXPECT_EQ(Read(rows.last(1).value()), (std::vector<uint8_t>{0x81}));
  EXPECT_EQ(Read(rows.subspan(3).value()), (std::vector<uint8_t>{0x81}));
  EXPECT_TRUE(rows.subspan(4).value().empty());
  EXPECT_EQ(Read(rows.subspan(1, 2).value()),
            (std::vector<uint8_t>{0x42, 0xa5}));
  EXPECT_EQ(rows.first(5).status(), IndexError::kOutOfRange);
  EXPECT_EQ(rows.last(5).status(), IndexError::kOutOfRange);
  EXPECT_EQ(rows.subspan(5).status(), IndexError::kOutOfRange);
  EXPECT_EQ(rows.subspan(5, 0).status(), IndexError::kOutOfRange);
  EXPECT_EQ(rows.subspan(2, 3).status(), IndexError::kOutOfRange);
  // Without an overflow of offset + count.
  EXPECT_EQ(rows.subspan(1, static_cast<size_t>(-1)).status(),
            IndexError::kOutOfRange);
}

TEST(FlashSpanTest, ComparesIterators) {
  flash_span<uint8_t> const rows(kRows);
  auto position = rows.begin();
  EXPECT_TRUE(position == rows.begin());
  EXPECT_TRUE(position != rows.end());
  ++position;
  EXPECT_EQ(*position, 0x42);
  EXPECT_FALSE(position == rows.begin());
}

}  // namespace
}  // namespace utl_test
