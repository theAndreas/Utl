// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// inplace_function: something to call, kept inside.

#include <gtest/gtest.h>
#include <utl/function_ref.h>
#include <utl/inplace_function.h>

#include <cstdint>
#include <type_traits>

namespace utl_test {
namespace {

using utl::inplace_function;

int Twice(int value) { return 2 * value; }

// Counts how many are alive and how each came to be, so that a test sees
// every copy, move and destruction an inplace_function makes. Not trivially
// copyable, so it goes through the code that manages one.
struct Counted {
  static int alive;
  static int copies;
  static int moves;

  explicit Counted(int value_in) : value(value_in) { ++alive; }
  Counted(Counted const& other) : value(other.value) {
    ++alive;
    ++copies;
  }
  Counted(Counted&& other) noexcept : value(other.value) {
    other.value = -1;
    ++alive;
    ++moves;
  }
  Counted& operator=(Counted const&) = delete;
  Counted& operator=(Counted&&) = delete;
  ~Counted() { --alive; }

  int operator()() const { return value; }

  static void Clear() {
    alive = 0;
    copies = 0;
    moves = 0;
  }

  int value;
};
int Counted::alive = 0;
int Counted::copies = 0;
int Counted::moves = 0;

struct NotCallable {};

// Makes an inplace_function of a lambda that is gone when it returns: what is
// kept must be a copy.
inplace_function<int()> MakeAdder(int offset) {
  int const base = Twice(5);
  return [base, offset] { return base + offset; };
}

// Empty by default, and copied and assigned as a value.
static_assert(std::is_default_constructible_v<inplace_function<void()>>);
static_assert(std::is_copy_constructible_v<inplace_function<void()>>);
static_assert(std::is_copy_assignable_v<inplace_function<void()>>);

// Keeps what can be called with Args and returns what converts to R.
static_assert(std::is_convertible_v<int (*)(int), inplace_function<int(int)>>);
static_assert(std::is_convertible_v<int (&)(int), inplace_function<int(int)>>);
static_assert(
    std::is_convertible_v<int (*)(int), inplace_function<int64_t(int16_t)>>);
static_assert(std::is_convertible_v<utl::function_ref<int(int)>,
                                    inplace_function<int(int)>>);
static_assert(!std::is_constructible_v<inplace_function<void()>, int>);
static_assert(!std::is_constructible_v<inplace_function<void()>, NotCallable>);
static_assert(!std::is_constructible_v<inplace_function<int()>, void (*)()>);
static_assert(!std::is_constructible_v<inplace_function<int(int)>, int (*)()>);
static_assert(!std::is_assignable_v<inplace_function<void()>&, NotCallable>);

TEST(InplaceFunctionTest, IsEmptyWithoutACallable) {
  inplace_function<int(int)> const by_default;
  inplace_function<int(int)> const of_null = nullptr;
  int (*const no_function)(int) = nullptr;
  inplace_function<int(int)> const of_null_pointer = no_function;
  // Of other types, which take a null pointer the same way.
  inplace_function<int64_t(int16_t)> const widened = no_function;
  inplace_function<void(int)> const dropped = no_function;
  EXPECT_FALSE(by_default);
  EXPECT_FALSE(of_null);
  EXPECT_FALSE(of_null_pointer);
  EXPECT_FALSE(widened);
  EXPECT_FALSE(dropped);
}

TEST(InplaceFunctionTest, CallOfAnEmptyOneGivesTheDefault) {
  inplace_function<int(int)> const empty;
  EXPECT_EQ(empty(3), 0);
  inplace_function<void()> const nothing;
  nothing();  // does nothing
}

TEST(InplaceFunctionTest, CallsAFunction) {
  inplace_function<int(int)> const by_name = Twice;
  inplace_function<int(int)> const by_pointer = &Twice;
  inplace_function<int64_t(int16_t)> const widened = Twice;
  inplace_function<void(int)> const dropped = Twice;
  EXPECT_TRUE(by_name);
  EXPECT_EQ(by_name(3), 6);
  EXPECT_EQ(by_pointer(4), 8);
  EXPECT_EQ(widened(int16_t{21}), int64_t{42});
  dropped(1);
}

TEST(InplaceFunctionTest, KeepsACopyOfTheLambda) {
  inplace_function<int()> const adder = MakeAdder(5);
  EXPECT_EQ(adder(), 15);  // the lambda and its captures are gone, the copy not

  int count = 0;
  auto counter = [count]() mutable { return ++count; };
  inplace_function<int()> const f = counter;
  EXPECT_EQ(f(), 1);
  EXPECT_EQ(f(), 2);        // the kept copy counts on, through a const one
  EXPECT_EQ(counter(), 1);  // the original is apart
}

TEST(InplaceFunctionTest, CopiesTheCallable) {
  int count = 0;
  inplace_function<int()> original = [count]() mutable { return ++count; };
  EXPECT_EQ(original(), 1);
  inplace_function<int()> const copy = original;
  inplace_function<int()> assigned;
  assigned = original;
  original = nullptr;  // the copies keep their own
  EXPECT_EQ(copy(), 2);
  EXPECT_EQ(copy(), 3);
  EXPECT_EQ(assigned(), 2);  // each counts on its own
}

TEST(InplaceFunctionTest, MovingLeavesTheSourceEmpty) {
  inplace_function<int(int)> source = Twice;
  inplace_function<int(int)> const moved =
      static_cast<inplace_function<int(int)>&&>(source);
  EXPECT_EQ(moved(2), 4);
  EXPECT_FALSE(source);  // NOLINT(bugprone-use-after-move)

  inplace_function<int(int)> target;
  source = Twice;
  target = static_cast<inplace_function<int(int)>&&>(source);
  EXPECT_EQ(target(3), 6);
  EXPECT_FALSE(source);  // NOLINT(bugprone-use-after-move)
}

TEST(InplaceFunctionTest, AssigningReplacesTheCallable) {
  inplace_function<int(int)> f = Twice;
  int const offset = Twice(1) - 1;
  f = [offset](int value) { return value + offset; };
  EXPECT_EQ(f(1), 2);
  f = nullptr;
  EXPECT_FALSE(f);
  f = &Twice;
  EXPECT_EQ(f(5), 10);
  inplace_function<int(int)> const& same = f;
  f = same;  // to itself
  EXPECT_EQ(f(5), 10);
  inplace_function<int(int)>& moved_same = f;
  f = static_cast<inplace_function<int(int)>&&>(moved_same);  // to itself
  EXPECT_EQ(f(5), 10);
}

TEST(InplaceFunctionTest, ManagesACallableThatIsNotTriviallyCopyable) {
  Counted::Clear();
  {
    Counted const counted(7);
    inplace_function<int(), sizeof(Counted)> first = counted;
    EXPECT_EQ(Counted::alive, 2);
    EXPECT_EQ(Counted::copies, 1);
    EXPECT_EQ(first(), 7);

    inplace_function<int(), sizeof(Counted)> second = first;
    EXPECT_EQ(Counted::alive, 3);
    EXPECT_EQ(second(), 7);

    inplace_function<int(), sizeof(Counted)> third =
        static_cast<inplace_function<int(), sizeof(Counted)>&&>(first);
    EXPECT_EQ(Counted::alive, 3);  // moved, and the moved-from one destroyed
    EXPECT_EQ(Counted::moves, 1);
    EXPECT_EQ(third(), 7);
    EXPECT_FALSE(first);  // NOLINT(bugprone-use-after-move)

    second = nullptr;
    EXPECT_EQ(Counted::alive, 2);
    third = second;  // empty over full: destroys
    EXPECT_EQ(Counted::alive, 1);
    second = Counted(9);  // moved in, the temporary gone
    EXPECT_EQ(second(), 9);
    EXPECT_EQ(Counted::alive, 2);
  }
  EXPECT_EQ(Counted::alive, 0);  // each destroyed once
}

TEST(InplaceFunctionTest, TakesALargerCallableWithMoreCapacity) {
  int64_t const a = Twice(1) - 1;
  int64_t const b = Twice(1);
  int64_t const c = Twice(1) + 1;
  inplace_function<int64_t(), 3 * sizeof(int64_t)> const sum = [a, b, c] {
    return a + b + c;
  };
  EXPECT_EQ(sum(), 6);
}

TEST(InplaceFunctionTest, KeepsAFunctionRef) {
  auto const add_one = [](int value) { return value + 1; };
  utl::function_ref<int(int)> const reference = add_one;
  inplace_function<int(int)> const kept = reference;
  EXPECT_EQ(kept(1), 2);
}

}  // namespace
}  // namespace utl_test
