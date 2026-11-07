// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// constant_wrapper and UTL_CW(): a value as a type.

#include <gtest/gtest.h>
#include <utl/constant_wrapper.h>

#include <cstdint>
#include <type_traits>

namespace utl_test {
namespace {

int Twice(int value) { return 2 * value; }

struct Counter {
  int Next() { return ++count; }
  int count{0};
};

template <typename A, typename B>
struct Pair {
  static int First() { return 1; }
};

constexpr uint8_t kAnswer{42};

// Nothing to store or pass.
static_assert(std::is_empty_v<utl::constant_wrapper<int, 1>>);
static_assert(std::is_trivially_copyable_v<utl::constant_wrapper<int, 1>>);

// The value and its type, from what UTL_CW() is given.
static_assert(decltype(UTL_CW(7))::value == 7);
static_assert(std::is_same_v<decltype(UTL_CW(7))::value_type, int>);
static_assert(std::is_same_v<decltype(UTL_CW(7))::type, decltype(UTL_CW(7))>);
// A constant without its const.
static_assert(std::is_same_v<decltype(UTL_CW(kAnswer)),
                             utl::constant_wrapper<uint8_t, 42>>);
// A function, by name or with &: a pointer to it either way.
static_assert(std::is_same_v<decltype(UTL_CW(Twice)),
                             utl::constant_wrapper<int (*)(int), &Twice>>);
static_assert(
    std::is_same_v<decltype(UTL_CW(&Twice)), decltype(UTL_CW(Twice))>);
// A member function.
static_assert(
    std::is_same_v<decltype(UTL_CW(&Counter::Next)),
                   utl::constant_wrapper<int (Counter::*)(), &Counter::Next>>);
// Commas in the argument.
static_assert(decltype(UTL_CW(&Pair<int, char>::First))::value ==
              &Pair<int, char>::First);

TEST(ConstantWrapperTest, GivesItsValue) {
  EXPECT_EQ(decltype(UTL_CW(Twice))::value(4), 8);
  Counter counter;
  EXPECT_EQ((counter.*decltype(UTL_CW(&Counter::Next))::value)(), 1);
  // Its value as an object, which needs the definition outside the class.
  uint8_t const& answer{decltype(UTL_CW(kAnswer))::value};
  EXPECT_EQ(answer, 42);
  EXPECT_EQ(&answer, &decltype(UTL_CW(kAnswer))::value);
}

}  // namespace
}  // namespace utl_test
