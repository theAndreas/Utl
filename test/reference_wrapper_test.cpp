// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// reference_wrapper: a reference that copies.

#include <gtest/gtest.h>
#include <utl/array.h>
#include <utl/reference_wrapper.h>

#include <type_traits>
#include <utility>

namespace utl_test {
namespace {

using utl::reference_wrapper;

struct Base {
  int id{0};
};
struct Derived : Base {};

// Takes an int&, to which a reference_wrapper<int> converts.
int* AddressOf(int& object) { return &object; }

// A class that refers to an object another owns, the use it is made for.
class Holder {
 public:
  explicit Holder(int& counter) : counter_{counter} {}
  void Increment() { ++counter_.get(); }
  [[nodiscard]] int const& counter() const { return counter_; }

 private:
  reference_wrapper<int> counter_;
};

// Whether ref() or cref() takes an argument of type T, so that the deleted
// overloads for an rvalue can be checked at compile time.
template <typename T, typename = void>
struct RefTakesImpl : std::false_type {};
template <typename T>
struct RefTakesImpl<T, std::void_t<decltype(utl::ref(std::declval<T>()))>>
    : std::true_type {};
template <typename T>
constexpr bool kRefTakes{RefTakesImpl<T>::value};

template <typename T, typename = void>
struct CrefTakesImpl : std::false_type {};
template <typename T>
struct CrefTakesImpl<T, std::void_t<decltype(utl::cref(std::declval<T>()))>>
    : std::true_type {};
template <typename T>
constexpr bool kCrefTakes{CrefTakesImpl<T>::value};

// A pointer, and nothing it owns: copied as one, without a destructor of its
// own.
static_assert(sizeof(reference_wrapper<int>) == sizeof(int*));
static_assert(std::is_trivially_copyable_v<reference_wrapper<int>>);
static_assert(std::is_trivially_destructible_v<reference_wrapper<int>>);

// Never null, and never bound to a temporary.
static_assert(!std::is_default_constructible_v<reference_wrapper<int>>);
static_assert(!std::is_constructible_v<reference_wrapper<int>, int>);
static_assert(!std::is_constructible_v<reference_wrapper<int const>, int>);
static_assert(!std::is_constructible_v<reference_wrapper<Base const>, Derived>);
static_assert(!kRefTakes<int> && !kRefTakes<int const>);
static_assert(!kCrefTakes<int> && !kCrefTakes<int const>);
static_assert(kRefTakes<int&> && kCrefTakes<int&>);

// Converts as a pointer does, and no further.
static_assert(std::is_convertible_v<reference_wrapper<int>,
                                    reference_wrapper<int const>>);
static_assert(
    std::is_convertible_v<reference_wrapper<Derived>, reference_wrapper<Base>>);
static_assert(!std::is_convertible_v<reference_wrapper<int const>,
                                     reference_wrapper<int>>);
static_assert(!std::is_convertible_v<reference_wrapper<Base>,
                                     reference_wrapper<Derived>>);
static_assert(!std::is_convertible_v<reference_wrapper<long>,  // NOLINT
                                     reference_wrapper<int>>);

// ref() and cref() take the type from the argument, and of a reference_wrapper
// give one of the same object, not one of a reference_wrapper.
static_assert(std::is_same_v<decltype(utl::ref(std::declval<int&>())),
                             reference_wrapper<int>>);
static_assert(std::is_same_v<decltype(utl::cref(std::declval<int&>())),
                             reference_wrapper<int const>>);
static_assert(
    std::is_same_v<decltype(utl::ref(std::declval<reference_wrapper<int>&>())),
                   reference_wrapper<int>>);
static_assert(
    std::is_same_v<decltype(utl::cref(std::declval<reference_wrapper<int>&>())),
                   reference_wrapper<int const>>);
static_assert(std::is_same_v<reference_wrapper<int>::type, int>);

// A reference_wrapper of an object with static storage is a constant.
int constant_target{0};
constexpr reference_wrapper<int> kConstant(constant_target);
static_assert(&kConstant.get() == &constant_target);

TEST(ReferenceWrapperTest, RefersToItsObject) {
  int value{3};
  reference_wrapper<int> const wrapper{value};
  EXPECT_EQ(&wrapper.get(), &value);
  wrapper.get() = 4;
  EXPECT_EQ(value, 4);
  EXPECT_EQ(AddressOf(wrapper), &value);  // passes as an int&
}

TEST(ReferenceWrapperTest, CopiesAndRebinds) {
  int first{1};
  int second{2};
  // = rather than braces: what is checked is that an lvalue converts
  // implicitly.
  reference_wrapper<int> wrapper = first;
  reference_wrapper<int> const copy{wrapper};
  EXPECT_EQ(&copy.get(), &first);
  wrapper = second;
  EXPECT_EQ(&wrapper.get(), &second);
  EXPECT_EQ(first, 1);  // assigning rebinds, it does not write
  EXPECT_EQ(&copy.get(), &first);
}

TEST(ReferenceWrapperTest, AsAMember) {
  int counter{0};
  int other{10};
  Holder holder{counter};
  holder.Increment();
  EXPECT_EQ(counter, 1);

  Holder copy{holder};  // copyable, as a T& member would not be
  copy.Increment();
  EXPECT_EQ(counter, 2);

  copy = Holder{other};  // assignable, as a T& member would not be
  copy.Increment();
  EXPECT_EQ(other, 11);
  EXPECT_EQ(&holder.counter(), &counter);
}

TEST(ReferenceWrapperTest, ConvertsAsAPointer) {
  Derived derived;
  // = rather than braces: what is checked is that it converts implicitly.
  reference_wrapper<Base> const base = derived;
  EXPECT_EQ(&base.get(), &derived);
  // = rather than braces: what is checked is that it converts implicitly.
  reference_wrapper<Base const> const from_wrapper = utl::ref(derived);
  EXPECT_EQ(&from_wrapper.get(), &derived);

  int value{5};
  // = rather than braces: what is checked is that it converts implicitly.
  reference_wrapper<int const> const read_only = utl::ref(value);
  EXPECT_EQ(&read_only.get(), &value);
}

TEST(ReferenceWrapperTest, RefAndCref) {
  int value{7};
  auto const wrapper = utl::ref(value);
  auto const read_only = utl::cref(value);
  EXPECT_EQ(&wrapper.get(), &value);
  EXPECT_EQ(&read_only.get(), &value);
  EXPECT_EQ(&utl::ref(wrapper).get(), &value);
  EXPECT_EQ(&utl::cref(wrapper).get(), &value);
}

TEST(ReferenceWrapperTest, InAnArray) {
  int red{1};
  int green{2};
  utl::array<reference_wrapper<int>, 2> const leds{{red, green}};
  for (int& led : leds) {
    led *= 10;
  }
  EXPECT_EQ(red, 10);
  EXPECT_EQ(green, 20);
}

}  // namespace
}  // namespace utl_test
