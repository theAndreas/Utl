// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// optional<T&>: a reference to a T that lies elsewhere, or none.

#include <gtest/gtest.h>
#include <utl/array.h>
#include <utl/optional.h>
#include <utl/optional_error.h>
#include <utl/result.h>

#include <type_traits>
#include <utility>

#include "test/alpha_error.h"

namespace utl_test {
namespace {

using utl::nullopt;
using utl::optional;

struct Base {
  int id{0};
};
struct Derived : Base {};

// A T without a default constructor, which an optional<T> could not hold.
class Led {
 public:
  explicit Led(int pin) : pin_{pin} {}
  void On() { on_ = true; }
  [[nodiscard]] int pin() const { return pin_; }
  [[nodiscard]] bool on() const { return on_; }

 private:
  int pin_;
  bool on_{false};
};

// Returns the element of `values` equal to `value`, or none.
optional<int&> Find(utl::array<int, 3>& values, int value) {
  for (int& element : values) {
    if (element == value) {
      return element;
    }
  }
  return nullopt;
}

// Whether ValueOr() takes an argument of type T, so that the deleted overload
// for a temporary can be checked at compile time.
template <typename T, typename = void>
struct ValueOrTakesImpl : std::false_type {};
template <typename T>
struct ValueOrTakesImpl<
    T, std::void_t<decltype(std::declval<optional<int&>>().ValueOr(
           std::declval<T>()))>> : std::true_type {};
template <typename T>
constexpr bool kValueOrTakes{ValueOrTakesImpl<T>::value};

// A pointer, null if empty, and nothing it owns.
static_assert(sizeof(optional<int&>) == sizeof(int*));
static_assert(std::is_trivially_copyable_v<optional<int&>>);
static_assert(std::is_same_v<optional<int&>::value_type, int>);

// Never bound to a temporary, nor given one as the fallback.
static_assert(std::is_constructible_v<optional<int&>, int&>);
static_assert(!std::is_constructible_v<optional<int&>, int>);
static_assert(!std::is_constructible_v<optional<int const&>, int>);
static_assert(!std::is_constructible_v<optional<Base const&>, Derived>);
static_assert(kValueOrTakes<int&> && !kValueOrTakes<int>);

// Converts as a pointer does, and no further.
static_assert(std::is_convertible_v<optional<int&>, optional<int const&>>);
static_assert(std::is_convertible_v<optional<Derived&>, optional<Base&>>);
static_assert(!std::is_convertible_v<optional<int const&>, optional<int&>>);
static_assert(!std::is_convertible_v<optional<Base&>, optional<Derived&>>);
static_assert(!std::is_convertible_v<optional<long&>,  // NOLINT
                                     optional<int&>>);

// One of an object with static storage is a constant.
int constant_target{0};
constexpr optional<int&> kConstant(constant_target);
static_assert(kConstant.HasValue() && &*kConstant == &constant_target);
static_assert(!optional<int&>{}.HasValue());

TEST(OptionalReferenceTest, EmptyOrAReference) {
  utl::array<int, 3> values{{0, 1, 7}};
  optional<int&> const none{Find(values, 42)};
  optional<int&> const found{Find(values, 7)};
  EXPECT_FALSE(none.HasValue());
  EXPECT_FALSE(static_cast<bool>(none));
  EXPECT_EQ(none, nullopt);
  EXPECT_TRUE(found.HasValue());
  EXPECT_EQ(&*found, &values[2]);
  EXPECT_EQ(found.operator->(), &values[2]);
}

TEST(OptionalReferenceTest, WritesThrough) {
  utl::array<int, 3> values{{0, 1, 7}};
  optional<int&> const found{Find(values, 7)};
  *found = 8;  // a const optional<int&> still gives an int&
  EXPECT_EQ(values[2], 8);

  Led led{13};
  optional<Led&> const some{led};
  some->On();
  EXPECT_TRUE(led.on());
  EXPECT_EQ((*some).pin(), 13);
}

TEST(OptionalReferenceTest, AssigningRebinds) {
  int first{1};
  int second{2};
  optional<int&> reference{first};
  reference = second;
  EXPECT_EQ(&*reference, &second);
  EXPECT_EQ(first, 1);  // rebound, not written

  optional<int&> const copy{reference};
  EXPECT_EQ(&*copy, &second);

  reference = nullopt;
  EXPECT_FALSE(reference.HasValue());
  EXPECT_EQ(&reference.emplace(first), &first);
  EXPECT_EQ(&*reference, &first);
  reference.reset();
  EXPECT_FALSE(reference.HasValue());
  reference = first;
  reference = {};
  EXPECT_FALSE(reference.HasValue());
}

TEST(OptionalReferenceTest, ConvertsAsAPointer) {
  Derived derived;
  optional<Derived&> const some{derived};
  optional<Base&> const base{some};
  EXPECT_EQ(&*base, &derived);
  optional<Derived&> const nothing;
  optional<Base&> const empty{nothing};
  EXPECT_FALSE(empty.HasValue());

  int value{4};
  optional<int const&> const read_only{optional<int&>{value}};
  EXPECT_EQ(&*read_only, &value);
  optional<int&> const no_int;
  optional<int const&> const none{no_int};
  EXPECT_FALSE(none.HasValue());
}

TEST(OptionalReferenceTest, ValueAndOkOrReturnAResult) {
  int target{3};
  optional<int&> const some{target};
  optional<int&> const none;
  utl::Result<int&> const value{some.value()};
  EXPECT_EQ(&value.value(), &target);
  EXPECT_EQ(none.value().status(), utl::OptionalError::kEmpty);
  EXPECT_EQ(&some.OkOr(AlphaError::kFirst).value(), &target);
  EXPECT_EQ(none.OkOr(AlphaError::kFirst).status(), AlphaError::kFirst);
}

TEST(OptionalReferenceTest, ValueOrGivesAReference) {
  Led led{13};
  Led spare{7};
  optional<Led&> const some{led};
  optional<Led&> const none;
  EXPECT_EQ(&some.ValueOr(spare), &led);
  none.ValueOr(spare).On();  // the spare itself, not a copy
  EXPECT_TRUE(spare.on());
}

TEST(OptionalReferenceTest, AndThen) {
  int target{3};
  optional<int&> const some{target};
  optional<int&> const none;
  auto const twice = [](int& value) { return optional<int>{value * 2}; };
  EXPECT_EQ(some.AndThen(twice), 6);
  EXPECT_EQ(none.AndThen(twice), nullopt);
}

TEST(OptionalReferenceTest, MapGetsTheReference) {
  int target{3};
  optional<int&> const some{target};
  some.Map([](int& value) { return value += 1; });
  EXPECT_EQ(target, 4);
  // A function that returns a reference gives an optional of one.
  optional<int&> const same{some.Map([](int& value) -> int& { return value; })};
  EXPECT_EQ(&*same, &target);
}

TEST(OptionalReferenceTest, MapCallsNothingWhenEmpty) {
  optional<int&> const none;
  int calls{0};
  optional<int> const mapped = none.Map([&calls](int&) {
    ++calls;
    return 1;
  });
  EXPECT_EQ(mapped, nullopt);
  EXPECT_EQ(calls, 0);
}

TEST(OptionalReferenceTest, OrElse) {
  int target{3};
  int other{9};
  optional<int&> const some{target};
  optional<int&> const none;
  auto const fallback = [&other] { return optional<int&>{other}; };
  EXPECT_EQ(&*none.OrElse(fallback), &other);
  EXPECT_EQ(&*some.OrElse(fallback), &target);
}

TEST(OptionalReferenceTest, ComparesTheObjects) {
  int a{5};
  int b{5};
  int c{6};
  optional<int&> const none;
  EXPECT_EQ(optional<int&>{a}, optional<int&>{b});  // equal values, two objects
  EXPECT_NE(optional<int&>{a}, optional<int&>{c});
  EXPECT_NE(optional<int&>{a}, none);
  EXPECT_NE(none, optional<int&>{a});
  EXPECT_EQ(none, optional<int&>{});
  EXPECT_TRUE(optional<int&>{a} == 5 && 5 == optional<int&>{a});
  EXPECT_TRUE(optional<int&>{a} != 6 && 6 != optional<int&>{a});
  EXPECT_TRUE(none != 5 && !(none == 5));
}

TEST(OptionalReferenceTest, ValueOrElseGivesAReference) {
  int target{3};
  int other{9};
  optional<int&> const some{target};
  optional<int&> const none;
  auto const fallback = [&other]() -> int& { return other; };
  EXPECT_EQ(&some.ValueOrElse(fallback), &target);
  EXPECT_EQ(&none.ValueOrElse(fallback), &other);
}

TEST(OptionalReferenceTest, InspectAndFilterPassTheReferenceOn) {
  int target{4};
  optional<int&> const some{target};
  optional<int&> const none;
  int seen{0};
  auto const look = [&seen](int const& value) { seen = value; };
  EXPECT_EQ(&*some.Inspect(look), &target);
  EXPECT_EQ(seen, 4);
  EXPECT_FALSE(none.Inspect(look).HasValue());
  auto const even = [](int const& value) { return value % 2 == 0; };
  EXPECT_EQ(&*some.Filter(even), &target);
  EXPECT_FALSE(some.Filter([](int const&) { return false; }).HasValue());
  EXPECT_FALSE(none.Filter(even).HasValue());
}

}  // namespace
}  // namespace utl_test
