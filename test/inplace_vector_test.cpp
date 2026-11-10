// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// inplace_vector: a vector of at most N elements, kept inside.

#include <gtest/gtest.h>
#include <utl/index_error.h>
#include <utl/inplace_vector.h>
#include <utl/span.h>
#include <utl/status.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

namespace utl_test {
namespace {

using utl::IndexError;
using utl::inplace_vector;

// An element that is not trivially copyable, and counts how many of it
// live, so that each made is destroyed exactly once.
struct Counted {
  static int alive;

  // Implicit, so that a braced list of ints makes elements.
  // NOLINTNEXTLINE(google-explicit-constructor)
  Counted(int value_in)  // NOLINT(runtime/explicit)
      : value{value_in} {
    ++alive;
  }
  Counted(Counted const& other) : value{other.value} { ++alive; }
  Counted(Counted&& other) noexcept : value{other.value} {
    other.value = -1;
    ++alive;
  }
  Counted& operator=(Counted const&) = default;
  Counted& operator=(Counted&& other) noexcept {
    value = other.value;
    other.value = -1;
    return *this;
  }
  ~Counted() { --alive; }

  bool operator==(Counted const& other) const { return value == other.value; }

  int value;
};
int Counted::alive = 0;

// No default constructor, which an element needs not have.
struct Pin {
  explicit Pin(uint8_t number_in) : number{number_in} {}
  uint8_t number;
};

using Bytes = inplace_vector<uint8_t, 4>;
using Objects = inplace_vector<Counted, 4>;

// The elements and a count of 1 byte; for trivially copyable elements, its
// bytes are copied, and nothing is destroyed.
static_assert(sizeof(Bytes) == 5);
static_assert(sizeof(inplace_vector<uint8_t, 255>) == 256);
static_assert(sizeof(inplace_vector<uint8_t, 256>) == 256 + sizeof(size_t));
static_assert(std::is_trivially_copyable_v<Bytes>);
static_assert(std::is_trivially_destructible_v<Bytes>);
static_assert(std::is_trivially_copyable_v<inplace_vector<Pin, 2>>);
static_assert(!std::is_trivially_copyable_v<Objects>);
static_assert(!std::is_trivially_destructible_v<Objects>);
static_assert(Bytes::capacity() == 4 && Bytes::max_size() == 4);

// Made empty when compiled.
constexpr Bytes kEmpty;
static_assert(kEmpty.empty() && kEmpty.size() == 0);  // NOLINT

// The sizes of a braced list and of a span.
uint16_t Sum(utl::span<uint8_t const> values) {
  uint16_t sum{0};
  for (uint8_t const value : values) {
    sum = static_cast<uint16_t>(sum + value);
  }
  return sum;
}

std::vector<int> Values(Objects const& objects) {
  std::vector<int> values;
  for (Counted const& object : objects) {
    values.push_back(object.value);
  }
  return values;
}

class InplaceVectorTest : public ::testing::Test {
 protected:
  void SetUp() override { Counted::alive = 0; }
  // Each element made was destroyed.
  void TearDown() override { EXPECT_EQ(Counted::alive, 0); }
};

TEST_F(InplaceVectorTest, IsMadeOfABracedList) {
  Bytes const bytes{2, 3, 4};
  EXPECT_EQ(bytes.size(), 3U);
  EXPECT_FALSE(bytes.empty());
  EXPECT_EQ(bytes[0], 2);
  EXPECT_EQ(bytes[2], 4);
  // = rather than braces: what is checked is that the constructor of a braced
  // list is implicit.
  Bytes assigned = {9};
  assigned = {5, 6};
  EXPECT_EQ(assigned, (Bytes{5, 6}));
  Bytes const none{};
  EXPECT_TRUE(none.empty());
}

TEST_F(InplaceVectorTest, AddsAtTheEndUntilFull) {
  Bytes bytes;
  EXPECT_TRUE(bytes.push_back(1).IsOk());
  uint8_t const two{2};
  EXPECT_TRUE(bytes.push_back(two).IsOk());
  EXPECT_TRUE(bytes.emplace_back(uint8_t{3}).IsOk());
  EXPECT_TRUE(bytes.emplace_back().IsOk());  // T(), 0
  EXPECT_EQ(bytes, (Bytes{1, 2, 3, 0}));
  EXPECT_EQ(bytes.push_back(5), IndexError::kFull);
  EXPECT_EQ(bytes.emplace_back(uint8_t{5}), IndexError::kFull);
  EXPECT_EQ(bytes.size(), 4U);  // as it was
}

TEST_F(InplaceVectorTest, MakesAnElementWithoutADefaultConstructor) {
  inplace_vector<Pin, 2> pins;
  EXPECT_TRUE(pins.emplace_back(uint8_t{13}).IsOk());
  EXPECT_TRUE(pins.push_back(Pin{7}).IsOk());
  EXPECT_EQ(pins.front().number, 13);
  EXPECT_EQ(pins.back().number, 7);
}

TEST_F(InplaceVectorTest, GivesItsElements) {
  Bytes bytes{2, 3, 4};
  Bytes const& const_bytes{bytes};
  EXPECT_EQ(bytes.at(1).value(), 3);
  EXPECT_EQ(bytes.at(3).status(), IndexError::kOutOfRange);  // not added
  EXPECT_EQ(bytes.front(), 2);
  EXPECT_EQ(const_bytes.front(), 2);
  EXPECT_EQ(bytes.back(), 4);
  EXPECT_EQ(const_bytes.back(), 4);
  EXPECT_EQ(bytes.data(), bytes.begin());
  EXPECT_EQ(const_bytes.data(), const_bytes.begin());
  bytes[0] = 7;
  bytes.front() = 8;
  bytes.back() = 9;
  EXPECT_EQ(bytes, (Bytes{8, 3, 9}));
  EXPECT_EQ(bytes.end() - bytes.begin(), 3);
  EXPECT_EQ(const_bytes.end() - const_bytes.begin(), 3);
  EXPECT_EQ(bytes.cend() - bytes.cbegin(), 3);
}

TEST_F(InplaceVectorTest, IsASpan) {
  Bytes bytes{2, 3, 4};
  EXPECT_EQ(Sum(bytes), 9);
  utl::span<uint8_t> const writable{bytes};
  writable[0] = 5;
  EXPECT_EQ(bytes[0], 5);
  Bytes const const_bytes{1};
  EXPECT_EQ(Sum(const_bytes), 1);
  static_assert(!std::is_convertible_v<Bytes const&, utl::span<uint8_t>>);
}

TEST_F(InplaceVectorTest, InsertsBeforeAPosition) {
  Bytes bytes{2, 4};
  EXPECT_TRUE(bytes.insert(bytes.begin() + 1, 3).IsOk());
  uint8_t const one{1};
  EXPECT_TRUE(bytes.insert(bytes.begin(), one).IsOk());
  EXPECT_EQ(bytes, (Bytes{1, 2, 3, 4}));
  EXPECT_EQ(bytes.insert(bytes.begin(), 0), IndexError::kFull);
  bytes.pop_back();
  EXPECT_TRUE(bytes.emplace(bytes.end(), uint8_t{5}).IsOk());  // at the end
  EXPECT_EQ(bytes, (Bytes{1, 2, 3, 5}));
}

TEST_F(InplaceVectorTest, InsertsNowhereOutside) {
  Bytes bytes{2, 3};
  // Within the room, past the elements.
  EXPECT_EQ(bytes.insert(bytes.end() + 1, 0), IndexError::kOutOfRange);
  EXPECT_EQ(bytes, (Bytes{2, 3}));
}

TEST_F(InplaceVectorTest, InsertsAnElementOfItself) {
  Objects objects{1, 2, 3};
  // The last, which moves while the others make room.
  EXPECT_TRUE(objects.insert(objects.begin(), objects.back()).IsOk());
  EXPECT_EQ(Values(objects), (std::vector<int>{3, 1, 2, 3}));
}

TEST_F(InplaceVectorTest, ErasesAndMovesTheRestBack) {
  Bytes bytes{1, 2, 3, 4};
  uint8_t const* const next{bytes.erase(bytes.begin() + 1)};
  EXPECT_EQ(*next, 3);
  EXPECT_EQ(bytes, (Bytes{1, 3, 4}));
  EXPECT_EQ(bytes.erase(bytes.begin(), bytes.begin() + 2), bytes.begin());
  EXPECT_EQ(bytes, (Bytes{4}));
  EXPECT_EQ(bytes.erase(bytes.begin(), bytes.begin()), bytes.begin());
  uint8_t const* const after_last{bytes.erase(bytes.begin())};  // the last
  EXPECT_EQ(after_last, bytes.end());
  EXPECT_TRUE(bytes.empty());
}

TEST_F(InplaceVectorTest, RemovesFromTheEnd) {
  Bytes bytes{1, 2, 3};
  bytes.pop_back();
  EXPECT_EQ(bytes, (Bytes{1, 2}));
  bytes.clear();
  EXPECT_TRUE(bytes.empty());
}

TEST_F(InplaceVectorTest, Resizes) {
  Bytes bytes{1, 2, 3};
  EXPECT_TRUE(bytes.resize(1).IsOk());
  EXPECT_EQ(bytes, (Bytes{1}));
  EXPECT_TRUE(bytes.resize(3).IsOk());
  EXPECT_EQ(bytes, (Bytes{1, 0, 0}));
  EXPECT_TRUE(bytes.resize(4, 9).IsOk());
  EXPECT_EQ(bytes, (Bytes{1, 0, 0, 9}));
  EXPECT_EQ(bytes.resize(5), IndexError::kFull);
  EXPECT_EQ(bytes.resize(5, 1), IndexError::kFull);
  EXPECT_EQ(bytes.size(), 4U);  // as it was
}

TEST_F(InplaceVectorTest, Compares) {
  EXPECT_EQ((Bytes{1, 2}), (Bytes{1, 2}));
  EXPECT_NE((Bytes{1, 2}), (Bytes{1, 3}));
  EXPECT_NE((Bytes{1, 2}), (Bytes{1}));
  EXPECT_FALSE((Bytes{1, 2}) != (Bytes{1, 2}));
}

TEST_F(InplaceVectorTest, MakesAndDestroysEachElementOnce) {
  {
    Objects objects{1, 2};
    EXPECT_EQ(Counted::alive, 2);
    EXPECT_TRUE(objects.push_back(Counted{3}).IsOk());
    EXPECT_EQ(Counted::alive, 3);
    objects.pop_back();
    EXPECT_EQ(Counted::alive, 2);
    EXPECT_TRUE(objects.resize(4, Counted{7}).IsOk());
    EXPECT_EQ(Values(objects), (std::vector<int>{1, 2, 7, 7}));
    objects.erase(objects.begin(), objects.begin() + 2);
    EXPECT_EQ(Values(objects), (std::vector<int>{7, 7}));
    EXPECT_EQ(Counted::alive, 2);
    EXPECT_TRUE(objects.insert(objects.begin() + 1, Counted{5}).IsOk());
    EXPECT_EQ(Values(objects), (std::vector<int>{7, 5, 7}));
    EXPECT_EQ(Counted::alive, 3);
    EXPECT_TRUE(objects.emplace(objects.end(), 6).IsOk());
    EXPECT_EQ(Values(objects), (std::vector<int>{7, 5, 7, 6}));
    EXPECT_NE(objects, (Objects{7, 5, 7, 8}));
    objects = {4};
    EXPECT_EQ(Counted::alive, 1);
  }
  EXPECT_EQ(Counted::alive, 0);
}

TEST_F(InplaceVectorTest, CopiesAndMovesElementByElement) {
  Objects const original{1, 2};
  Objects copy{original};
  EXPECT_EQ(copy, original);
  EXPECT_EQ(Counted::alive, 4);
  Objects const moved{std::move(copy)};
  EXPECT_EQ(Values(moved), (std::vector<int>{1, 2}));

  Objects assigned{9, 9, 9};
  assigned = original;
  EXPECT_EQ(assigned, original);
  assigned = Objects{5};
  EXPECT_EQ(Values(assigned), (std::vector<int>{5}));
  Objects const& same{assigned};
  assigned = same;  // to itself
  EXPECT_EQ(Values(assigned), (std::vector<int>{5}));
  Objects& alias{assigned};
  assigned = std::move(alias);  // to itself, which keeps it
  EXPECT_EQ(Values(assigned), (std::vector<int>{5}));
}

TEST_F(InplaceVectorTest, CopiesTriviallyCopyableElementsAsBytes) {
  Bytes const original{1, 2};
  Bytes copy{original};
  EXPECT_EQ(copy, original);
  copy = Bytes{3};
  EXPECT_EQ(copy, (Bytes{3}));
}

}  // namespace
}  // namespace utl_test
