// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// function_ref: a reference to something to call.

#include <gtest/gtest.h>
#include <utl/constant_wrapper.h>
#include <utl/function_ref.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace utl_test {
namespace {

using utl::function_ref;

int Twice(int value) { return 2 * value; }
void Nothing() {}
int calls{0};
void CountCall() { ++calls; }

// Calls f once per value, the use it is made for: a parameter, which takes a
// function, a lambda or an object without being a template.
int Sum(function_ref<int(int)> f) {
  int sum{0};
  for (int value{1}; value <= 3; ++value) {
    sum += f(value);
  }
  return sum;
}

// Which operator() is called tells whether f was const.
struct Overloaded {
  int operator()() { return 1; }
  int operator()() const { return 2; }
};

// Callable, but only with an int.
struct TakesInt {
  // Never called: only its signature is asked for.
  [[maybe_unused]] void operator()(int /*value*/) const {}
};

// A result that converts to an R only explicitly.
struct Explicit {
  // Never called: only whether an int converts is asked for.
  [[maybe_unused]] explicit Explicit(int /*value*/) {}
};

struct NotCallable {};

// A callable that overloads unary &, which must not be taken for its
// address.
struct AddressHidden {
  int operator()() const { return 5; }
  // NOLINTNEXTLINE(google-runtime-operator)
  void operator&() const = delete;  // NOLINT(runtime/operator)
};

// Two pointers, and nothing it owns: copied as they are, without a destructor
// of its own.
static_assert(sizeof(function_ref<void()>) == 2 * sizeof(void*));
static_assert(std::is_trivially_copyable_v<function_ref<void()>>);
static_assert(std::is_trivially_destructible_v<function_ref<void()>>);

// Never empty.
static_assert(!std::is_default_constructible_v<function_ref<void()>>);
static_assert(!std::is_constructible_v<function_ref<void()>, std::nullptr_t>);

// Binds what can be called with Args and returns what converts to R.
static_assert(std::is_convertible_v<int (*)(int), function_ref<int(int)>>);
static_assert(std::is_convertible_v<int (&)(int), function_ref<int(int)>>);
static_assert(std::is_convertible_v<Overloaded&, function_ref<int()>>);
static_assert(
    std::is_convertible_v<int (*)(int), function_ref<int64_t(int16_t)>>);
static_assert(std::is_convertible_v<int (*)(int), function_ref<void(int)>>);
static_assert(!std::is_constructible_v<function_ref<void()>, int>);
static_assert(!std::is_constructible_v<function_ref<void()>, NotCallable>);
static_assert(!std::is_constructible_v<function_ref<void()>, TakesInt>);
static_assert(!std::is_constructible_v<function_ref<void(int*)>, TakesInt>);
static_assert(!std::is_constructible_v<function_ref<int()>, void (*)()>);
static_assert(!std::is_constructible_v<function_ref<Explicit()>, int (*)()>);
static_assert(!std::is_constructible_v<function_ref<int(int)>, int (*)()>);

// Assigned only another function_ref or a function, never a lambda or an
// object, which would be gone before the next call.
static_assert(
    std::is_assignable_v<function_ref<void()>&, function_ref<void()>>);
static_assert(std::is_assignable_v<function_ref<void()>&, void (*)()>);
static_assert(std::is_assignable_v<function_ref<void()>&, void (&)()>);
static_assert(!std::is_assignable_v<function_ref<void()>&, std::nullptr_t>);

TEST(FunctionRefTest, BindsALambdaButIsNeverAssignedOne) {
  auto const nothing = [] {};
  using Lambda = std::remove_const_t<decltype(nothing)>;
  static_assert(std::is_convertible_v<Lambda, function_ref<void()>>);
  static_assert(!std::is_assignable_v<function_ref<void()>&, Lambda>);
  static_assert(!std::is_assignable_v<function_ref<void()>&, Lambda&>);
  function_ref<void()> const f{nothing};
  f();
}

TEST(FunctionRefTest, CallsAFunction) {
  function_ref<int(int)> const by_name{Twice};
  function_ref<int(int)> const by_pointer{&Twice};
  EXPECT_EQ(by_name(3), 6);
  EXPECT_EQ(by_pointer(4), 8);
  EXPECT_EQ(Sum(Twice), 12);
}

TEST(FunctionRefTest, CallsAFunctionOfOtherTypes) {
  function_ref<int64_t(int16_t)> const widened{Twice};
  EXPECT_EQ(widened(int16_t{21}), int64_t{42});
  function_ref<void(int)> const dropped{Twice};  // the result is dropped
  dropped(1);
}

TEST(FunctionRefTest, CallsALambdaWithItsCaptures) {
  int offset{10};
  auto const add = [&offset](int value) { return value + offset; };
  function_ref<int(int)> const f{add};
  EXPECT_EQ(f(1), 11);
  offset = 20;  // seen, as the lambda is referred to, not copied
  EXPECT_EQ(f(1), 21);
  EXPECT_EQ(Sum([](int value) { return value * value; }), 14);
}

TEST(FunctionRefTest, RefersToTheObjectNotACopy) {
  int count{0};
  auto counter = [count]() mutable { return ++count; };
  function_ref<int()> const f{counter};
  EXPECT_EQ(f(), 1);
  EXPECT_EQ(f(), 2);
  EXPECT_EQ(counter(), 3);  // the same object, counting on
}

TEST(FunctionRefTest, CallsConstOperatorOfAConstObject) {
  Overloaded mutable_object;
  Overloaded const const_object;
  function_ref<int()> const f{mutable_object};
  function_ref<int()> const g{const_object};
  EXPECT_EQ(f(), 1);
  EXPECT_EQ(g(), 2);
}

TEST(FunctionRefTest, TakesTheAddressPastAnOverloadedAmpersand) {
  AddressHidden const hidden;
  function_ref<int()> const f{hidden};
  EXPECT_EQ(f(), 5);
}

TEST(FunctionRefTest, PassesArgumentsAsTaken) {
  auto const increment = [](int& value) { ++value; };
  function_ref<void(int&)> const f{increment};
  int value{1};
  f(value);
  EXPECT_EQ(value, 2);

  // Moved through to the callable, never copied.
  auto const take = [](std::unique_ptr<int> owned) { return *owned; };
  function_ref<int(std::unique_ptr<int>)> const g{take};
  EXPECT_EQ(g(std::make_unique<int>(7)), 7);
}

TEST(FunctionRefTest, ReturnsAReference) {
  int target{1};
  auto const get = [&target]() -> int& { return target; };
  function_ref<int&()> const f{get};
  f() = 5;
  EXPECT_EQ(target, 5);
  EXPECT_EQ(&f(), &target);
}

TEST(FunctionRefTest, CopiesAndRebinds) {
  calls = 0;
  function_ref<void()> f{Nothing};
  function_ref<void()> const copy{f};
  f = CountCall;  // a function, assigned by name
  f();
  copy();  // still Nothing
  EXPECT_EQ(calls, 1);
  auto const twice = [] {
    CountCall();
    CountCall();
  };
  f = function_ref<void()>(twice);
  f();
  EXPECT_EQ(calls, 3);
}

// A class with member functions to bind: const and not, of other types, and
// one overloaded on const.
class Led {
 public:
  void Toggle() { on_ = !on_; }
  [[nodiscard]] bool IsOn() const { return on_; }
  [[nodiscard]] int Brightness(int scale) const { return on_ ? scale : 0; }
  // Which one was called: the one not const switches the LED on.
  int Which() {
    on_ = true;
    return 1;
  }
  [[nodiscard]] int Which() const { return on_ ? 2 : -2; }

 private:
  bool on_{false};
};

class DimmedLed : public Led {};

// A data member, which is never called.
struct Reading {
  int level{0};
};

// A function that takes its object first, as a member function would.
int Scaled(Led const& led, int scale) { return led.Brightness(scale) / 2; }
int Count(Led const* led) { return led == nullptr ? -1 : 1; }

using ToggleWrapper = decltype(UTL_CW(&Led::Toggle));
using IsOnWrapper = decltype(UTL_CW(&Led::IsOn));

// Bound to an lvalue or a pointer, never a temporary.
static_assert(
    std::is_constructible_v<function_ref<void()>, ToggleWrapper, Led&>);
static_assert(
    std::is_constructible_v<function_ref<void()>, ToggleWrapper, Led*>);
static_assert(
    !std::is_constructible_v<function_ref<void()>, ToggleWrapper, Led&&>);
static_assert(
    !std::is_constructible_v<function_ref<void()>, ToggleWrapper, Led const&>);
static_assert(
    std::is_constructible_v<function_ref<bool()>, IsOnWrapper, Led const&>);
static_assert(
    std::is_constructible_v<function_ref<bool()>, IsOnWrapper, Led const*>);
static_assert(
    std::is_constructible_v<function_ref<void()>, ToggleWrapper, DimmedLed&>);
// Of the right types only, as any other callable.
static_assert(
    !std::is_constructible_v<function_ref<void(int)>, ToggleWrapper, Led&>);
static_assert(
    !std::is_constructible_v<function_ref<int()>, ToggleWrapper, Led&>);
static_assert(
    !std::is_constructible_v<function_ref<void()>, ToggleWrapper, int&>);
static_assert(
    !std::is_constructible_v<function_ref<void()>, decltype(UTL_CW(7)), Led&>);
static_assert(
    !std::is_constructible_v<function_ref<void()>, decltype(UTL_CW(7))>);
// Without an object, the member function takes it as its first argument.
static_assert(std::is_convertible_v<ToggleWrapper, function_ref<void(Led&)>>);
static_assert(std::is_convertible_v<ToggleWrapper, function_ref<void(Led*)>>);
static_assert(!std::is_convertible_v<ToggleWrapper, function_ref<void()>>);
// A data member is not called.
static_assert(!std::is_convertible_v<decltype(UTL_CW(&Reading::level)),
                                     function_ref<int(Reading&)>>);

TEST(FunctionRefTest, CallsAMemberFunctionOfItsObject) {
  Led led;
  function_ref<void()> const toggle{UTL_CW(&Led::Toggle), led};
  function_ref<bool()> const is_on{UTL_CW(&Led::IsOn), led};
  toggle();
  EXPECT_TRUE(led.IsOn());  // the object itself, not a copy
  toggle();
  EXPECT_FALSE(is_on());
}

TEST(FunctionRefTest, CallsAMemberFunctionThroughAPointer) {
  Led led;
  Led* pointer{&led};
  function_ref<void()> const toggle{UTL_CW(&Led::Toggle), pointer};
  Led other;
  pointer = &other;  // the pointer was copied, not referred to
  toggle();
  EXPECT_TRUE(led.IsOn());
  EXPECT_FALSE(pointer->IsOn());  // other
}

TEST(FunctionRefTest, CallsTheConstMemberFunctionOfAConstObject) {
  Led led;
  Led const& const_led{led};
  function_ref<int()> const mutable_which{
      UTL_CW(static_cast<int (Led::*)()>(&Led::Which)), led};
  function_ref<int()> const const_which{
      UTL_CW(static_cast<int (Led::*)() const>(&Led::Which)), const_led};
  EXPECT_EQ(mutable_which(), 1);
  EXPECT_EQ(const_which(), 2);
}

TEST(FunctionRefTest, CallsAMemberFunctionOfOtherTypes) {
  Led led;
  led.Toggle();
  function_ref<int64_t(int16_t)> const brightness{UTL_CW(&Led::Brightness),
                                                  led};
  EXPECT_EQ(brightness(int16_t{100}), int64_t{100});
  function_ref<void()> const dropped{UTL_CW(&Led::IsOn), led};
  dropped();
  DimmedLed dimmed;
  function_ref<void()> const of_base{UTL_CW(&Led::Toggle), dimmed};
  of_base();
  EXPECT_TRUE(dimmed.IsOn());
}

// A virtual member function, which a pointer to a member calls through the
// vtable. No move of its own: a move is its copy.
class Shape {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  Shape() = default;
  Shape(Shape const&) = default;
  Shape& operator=(Shape const&) = default;
  virtual ~Shape() = default;
  [[nodiscard]] virtual int Corners() const { return 0; }
};

class Square : public Shape {
 public:
  [[nodiscard]] int Corners() const override { return 4; }
};

TEST(FunctionRefTest, CallsAVirtualMemberFunctionOfTheDerivedClass) {
  Square const square;
  Shape const& shape{square};
  function_ref<int()> const of_reference{UTL_CW(&Shape::Corners), shape};
  function_ref<int()> const of_pointer{UTL_CW(&Shape::Corners), &shape};
  EXPECT_EQ(of_reference(), 4);
  EXPECT_EQ(of_pointer(), 4);
}

TEST(FunctionRefTest, CallsAMemberFunctionOnTheFirstArgument) {
  Led first;
  Led second;
  function_ref<void(Led&)> const toggle{UTL_CW(&Led::Toggle)};
  function_ref<void(Led*)> const toggle_pointed{UTL_CW(&Led::Toggle)};
  toggle(first);
  toggle_pointed(&second);
  EXPECT_TRUE(first.IsOn());
  EXPECT_TRUE(second.IsOn());
}

TEST(FunctionRefTest, BindsAnObjectToAFunctionThatTakesItFirst) {
  Led led;
  led.Toggle();
  function_ref<int(int)> const scaled{UTL_CW(Scaled), led};
  EXPECT_EQ(scaled(100), 50);
  EXPECT_EQ(Sum(scaled), 2);  // 0 + 1 + 1
  // A pointer that a function, not a member function, takes may be null.
  Led const* const none{nullptr};
  function_ref<int()> const count{UTL_CW(Count), none};
  EXPECT_EQ(count(), -1);
}

TEST(FunctionRefTest, CallsAFunctionGivenAsAConstant) {
  function_ref<int(int)> const twice{UTL_CW(Twice)};
  EXPECT_EQ(twice(5), 10);
  EXPECT_EQ(Sum(UTL_CW(Twice)), 12);
  // An object, not the declaration of a function, as UTL_CW() gives braces.
  function_ref<int(int)> const in_parentheses(UTL_CW(Twice));
  EXPECT_EQ(in_parentheses(1), 2);
}

TEST(FunctionRefTest, IsAConstantOfAMemberFunctionAndAnObject) {
  static Led led;
  static constexpr function_ref<void()> kToggle{UTL_CW(&Led::Toggle), led};
  static constexpr function_ref<int(int)> kTwice{UTL_CW(Twice)};
  kToggle();
  EXPECT_TRUE(led.IsOn());
  EXPECT_EQ(kTwice(2), 4);
}

TEST(FunctionRefTest, IsAConstantOfAnObjectWithStaticStorage) {
  static constexpr Overloaded kObject{};
  constexpr function_ref<int()> kF{kObject};
  EXPECT_EQ(kF(), 2);
}

}  // namespace
}  // namespace utl_test
