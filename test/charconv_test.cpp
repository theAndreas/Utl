// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// charconv: integers to text and back.

#include <gtest/gtest.h>
#include <utl/array.h>
#include <utl/charconv.h>
#include <utl/charconv_error.h>
#include <utl/status.h>
#include <utl/string_view.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

namespace utl_test {
namespace {

using utl::CharconvError;

// What to_chars() writes for `value`, or the error as "error".
template <typename T>
std::string Written(T value, uint8_t base = 10, size_t size = 24) {
  utl::array<char, 24> buffer{};
  auto const end =
      utl::to_chars(buffer.data(), buffer.data() + size, value, base);
  if (!end.IsOk()) {
    return "error";
  }
  return std::string(buffer.data(), end.value());
}

// What from_chars() reads from `text`, and how many chars it took.
template <typename T>
struct Read {
  utl::Status status;
  T value{};
  size_t taken{0};
};
template <typename T>
Read<T> ReadFrom(char const* text, uint8_t base = 10, T initial = T{7}) {
  T value{initial};
  size_t const size{std::strlen(text)};
  auto const end = utl::from_chars(text, text + size, value, base);
  size_t const taken{end.IsOk() ? static_cast<size_t>(end.value() - text)
                                : size_t{0}};
  return Read<T>{end.status(), value, taken};
}

TEST(ToCharsTest, WritesInDecimal) {
  EXPECT_EQ(Written(uint8_t{0}), "0");
  EXPECT_EQ(Written(uint8_t{42}), "42");
  EXPECT_EQ(Written(uint8_t{255}), "255");
  EXPECT_EQ(Written(int8_t{-128}), "-128");
  EXPECT_EQ(Written(int8_t{127}), "127");
  EXPECT_EQ(Written(int16_t{-32768}), "-32768");
  EXPECT_EQ(Written(uint16_t{65535}), "65535");
  EXPECT_EQ(Written(std::numeric_limits<int64_t>::min()),
            "-9223372036854775808");
  EXPECT_EQ(Written(std::numeric_limits<uint64_t>::max()),
            "18446744073709551615");
}

TEST(ToCharsTest, WritesInAnyBase) {
  EXPECT_EQ(Written(uint8_t{255}, 2), "11111111");
  EXPECT_EQ(Written(uint8_t{255}, 16), "ff");
  EXPECT_EQ(Written(int16_t{-255}, 16), "-ff");
  EXPECT_EQ(Written(uint8_t{35}, 36), "z");
  EXPECT_EQ(Written('A', 10),
            "65");  // a char is an integer, as in the standard
}

TEST(ToCharsTest, ValueTooLarge) {
  EXPECT_EQ(Written(uint8_t{255}, 10, 3), "255");  // just fits
  EXPECT_EQ(Written(uint8_t{255}, 10, 2), "error");
  EXPECT_EQ(Written(int8_t{-5}, 10, 1), "error");  // no room after the minus
  EXPECT_EQ(Written(int8_t{-5}, 10, 0), "error");  // no room for the minus
  utl::array<char, 2> buffer{};
  EXPECT_EQ(utl::to_chars(buffer.data(), buffer.data() + 2, 100).status(),
            CharconvError::kValueTooLarge);
}

TEST(ToCharsTest, InvalidBase) {
  utl::array<char, 4> buffer{};
  EXPECT_EQ(utl::to_chars(buffer.data(), buffer.data() + 4, 1, 1).status(),
            CharconvError::kInvalidArgument);
  EXPECT_EQ(utl::to_chars(buffer.data(), buffer.data() + 4, 1, 37).status(),
            CharconvError::kInvalidArgument);
}

TEST(ToCharsTest, IntoASpanGivesTheText) {
  utl::array<char, 6> buffer{};
  EXPECT_EQ(utl::to_chars(buffer, -273).ValueOr("x"), "-273");
  EXPECT_EQ(utl::to_chars(buffer, 0x1f, 16).ValueOr("x"), "1f");
  EXPECT_EQ(utl::to_chars(buffer, 1234567).status(),
            CharconvError::kValueTooLarge);
}

TEST(FromCharsTest, ReadsAndStopsAfterTheDigits) {
  Read<uint8_t> const read{ReadFrom<uint8_t>("42,7")};
  EXPECT_TRUE(read.status.IsOk());
  EXPECT_EQ(read.value, 42);
  EXPECT_EQ(read.taken, 2U);
  EXPECT_EQ(ReadFrom<int8_t>("-128").value, -128);
  EXPECT_EQ(ReadFrom<int8_t>("127").value, 127);
  EXPECT_EQ(ReadFrom<uint16_t>("65535").value, 65535);
  EXPECT_EQ(ReadFrom<int64_t>("-9223372036854775808").value,
            std::numeric_limits<int64_t>::min());
  EXPECT_EQ(ReadFrom<uint64_t>("18446744073709551615").value,
            std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(ReadFrom<int>("007").value, 7);
  EXPECT_EQ(ReadFrom<int>("-0").value, 0);
}

TEST(FromCharsTest, ReadsInAnyBase) {
  EXPECT_EQ(ReadFrom<uint8_t>("ff", 16).value, 255);
  EXPECT_EQ(ReadFrom<uint8_t>("FF", 16).value, 255);  // either case
  EXPECT_EQ(ReadFrom<uint8_t>("1012", 2).value, 5);   // stops at the 2
  EXPECT_EQ(ReadFrom<uint8_t>("1012", 2).taken, 3U);
  EXPECT_EQ(ReadFrom<uint8_t>("z", 36).value, 35);
  // "0x" is no prefix: the 0 is read, and it stops at the x.
  EXPECT_EQ(ReadFrom<uint8_t>("0x1f", 16).taken, 1U);
}

TEST(FromCharsTest, InvalidArgumentLeavesTheValue) {
  for (char const* text : {"", "-", "+1", " 1", "x", "-x", "[", "~"}) {
    Read<int> const read{ReadFrom<int>(text)};
    EXPECT_EQ(read.status, CharconvError::kInvalidArgument) << text;
    EXPECT_EQ(read.value, 7) << text;
  }
}

TEST(FromCharsTest, NoMinusForAnUnsignedAndNoBadBase) {
  // As in the standard.
  EXPECT_EQ(ReadFrom<uint8_t>("-1").status, CharconvError::kInvalidArgument);
  EXPECT_EQ(ReadFrom<uint8_t>("1", 1).status, CharconvError::kInvalidArgument);
  EXPECT_EQ(ReadFrom<uint8_t>("1", 37).status, CharconvError::kInvalidArgument);
}

TEST(FromCharsTest, OutOfRangeLeavesTheValue) {
  for (char const* text : {"256", "1000", "99999999999999999999"}) {
    Read<uint8_t> const read{ReadFrom<uint8_t>(text)};
    EXPECT_EQ(read.status, CharconvError::kOutOfRange) << text;
    EXPECT_EQ(read.value, 7) << text;
  }
}

TEST(FromCharsTest, OutOfRangeAtEachEnd) {
  EXPECT_EQ(ReadFrom<int8_t>("128").status, CharconvError::kOutOfRange);
  EXPECT_EQ(ReadFrom<int8_t>("-129").status, CharconvError::kOutOfRange);
  EXPECT_EQ(ReadFrom<uint8_t>("100000000", 2).status,
            CharconvError::kOutOfRange);
  EXPECT_EQ(ReadFrom<uint64_t>("18446744073709551616").status,
            CharconvError::kOutOfRange);
}

TEST(FromCharsTest, FromAStringViewTakesWhatItReads) {
  utl::string_view line{"42,-7"};
  EXPECT_EQ(utl::from_chars<uint8_t>(line).ValueOr(0), 42);
  EXPECT_EQ(line, ",-7");
  line.remove_prefix(1);
  EXPECT_EQ(utl::from_chars<int8_t>(line).ValueOr(0), -7);
  EXPECT_TRUE(line.empty());

  // On an error the view stays as it is.
  utl::string_view rest{"x1"};
  EXPECT_EQ(utl::from_chars<int>(rest).status(),
            CharconvError::kInvalidArgument);
  EXPECT_EQ(rest, "x1");
  utl::string_view large{"300"};
  EXPECT_EQ(utl::from_chars<uint8_t>(large).status(),
            CharconvError::kOutOfRange);
  EXPECT_EQ(large, "300");
}

// Every value of a type of up to 16 bits, in each base that matters on a
// panel, written and read back.
template <typename T>
void RoundTrip(uint8_t base) {
  using Limits = std::numeric_limits<T>;
  // Sign-extended on purpose: every value of T, as an int64_t.
  // NOLINTNEXTLINE(bugprone-signed-char-misuse)
  for (int64_t i{Limits::min()}; i <= Limits::max(); ++i) {
    T const value{static_cast<T>(i)};
    utl::array<char, 20> buffer{};
    char* const end{
        utl::to_chars(buffer.data(), buffer.data() + buffer.size(), value, base)
            .value()};
    T back{};
    ASSERT_EQ(utl::from_chars(buffer.data(), end, back, base).value(), end);
    ASSERT_EQ(back, value) << i << " base " << base;
  }
}

TEST(CharconvTest, RoundTripsEveryValue) {
  for (uint8_t const base :
       {uint8_t{2}, uint8_t{10}, uint8_t{16}, uint8_t{36}}) {
    RoundTrip<int8_t>(base);
    RoundTrip<uint8_t>(base);
    RoundTrip<int16_t>(base);
    RoundTrip<uint16_t>(base);
  }
}

}  // namespace
}  // namespace utl_test
