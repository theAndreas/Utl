// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// spsc_queue: a queue from one producer to one consumer. On the host these
// tests see its order and its bounds on one thread; SelfCheck in simavr sees
// it between an interrupt and the code it interrupts.

#include <gtest/gtest.h>
#include <utl/index_error.h>
#include <utl/optional.h>
#include <utl/spsc_queue.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace utl_test {
namespace {

using utl::IndexError;
using utl::spsc_queue;

struct Event {
  uint8_t kind;
  uint16_t value;
};

using Keys = spsc_queue<uint8_t, 4>;

// The elements and two counts of a byte.
static_assert(sizeof(spsc_queue<uint8_t, 16>) == 18);
static_assert(Keys::capacity() == 4);
static_assert(spsc_queue<uint8_t, 1>::capacity() == 1);
static_assert(spsc_queue<uint8_t, 128>::capacity() == 128);
// The two sides refer to one queue, never to copies.
static_assert(!std::is_copy_constructible_v<Keys>);
static_assert(!std::is_copy_assignable_v<Keys>);

TEST(SpscQueueTest, IsEmptyWhenMade) {
  Keys const keys;
  EXPECT_TRUE(keys.empty());
  EXPECT_FALSE(keys.full());
  EXPECT_EQ(keys.size(), 0U);
  EXPECT_FALSE(keys.front().HasValue());
}

TEST(SpscQueueTest, TakesInTheOrderAdded) {
  Keys keys;
  EXPECT_TRUE(keys.push(1).IsOk());
  EXPECT_TRUE(keys.push(2).IsOk());
  EXPECT_EQ(keys.size(), 2U);
  EXPECT_EQ(*keys.front(), 1);
  EXPECT_EQ(keys.size(), 2U);  // front() leaves it in
  EXPECT_EQ(*keys.pop(), 1);
  EXPECT_EQ(*keys.pop(), 2);
  EXPECT_FALSE(keys.pop().HasValue());
  EXPECT_TRUE(keys.empty());
}

// Pushes `first` on, one by one, while the queue takes them; returns the next
// one not pushed.
template <size_t N>
int PushFrom(spsc_queue<uint8_t, N>& queue, int first, int last) {
  int next = first;
  while (next < last && queue.push(static_cast<uint8_t>(next)).IsOk()) {
    ++next;
  }
  return next;
}

// Pops each, and returns how many came in order from `first` on.
template <size_t N>
int PopInOrder(spsc_queue<uint8_t, N>& queue, int first) {
  int next = first;
  while (utl::optional<uint8_t> const value = queue.pop()) {
    if (*value != static_cast<uint8_t>(next)) {
      break;
    }
    ++next;
  }
  return next - first;
}

TEST(SpscQueueTest, LeavesOutWhatComesWhenFull) {
  Keys keys;
  EXPECT_EQ(PushFrom(keys, 0, 9), 4);
  EXPECT_TRUE(keys.full());
  EXPECT_EQ(keys.push(9), IndexError::kFull);
  EXPECT_EQ(keys.size(), 4U);
  EXPECT_EQ(*keys.pop(), 0);
  EXPECT_FALSE(keys.full());
  EXPECT_TRUE(keys.push(4).IsOk());  // in the place freed
  EXPECT_EQ(PopInOrder(keys, 1), 4);
}

TEST(SpscQueueTest, CountsOnPastTheWrapOfAByte) {
  // 300 through a queue of 4, three at a time: the counts wrap at 256, and
  // their difference still says how many it holds.
  Keys keys;
  int next_in = 0;
  int next_out = 0;
  bool sizes_right = true;
  while (next_out < 300) {
    next_in = PushFrom(keys, next_in, std::min(next_in + 3, 300));
    sizes_right =
        sizes_right && keys.size() == static_cast<size_t>(next_in - next_out);
    int const popped = PopInOrder(keys, next_out);
    ASSERT_EQ(popped, next_in - next_out);
    next_out += popped;
  }
  EXPECT_TRUE(sizes_right);
  EXPECT_TRUE(keys.empty());
}

TEST(SpscQueueTest, FillsAQueueOf128) {
  spsc_queue<uint8_t, 128> bytes;
  EXPECT_EQ(PushFrom(bytes, 0, 200), 128);
  EXPECT_EQ(bytes.size(), 128U);
  EXPECT_TRUE(bytes.full());
  EXPECT_EQ(bytes.push(0), IndexError::kFull);
  EXPECT_EQ(*bytes.pop(), 0);
}

TEST(SpscQueueTest, ClearsWhatItHolds) {
  Keys keys;
  EXPECT_TRUE(keys.push(1).IsOk());
  EXPECT_TRUE(keys.push(2).IsOk());
  keys.clear();
  EXPECT_TRUE(keys.empty());
  EXPECT_TRUE(keys.push(3).IsOk());
  EXPECT_EQ(*keys.pop(), 3);
}

TEST(SpscQueueTest, CarriesAStruct) {
  spsc_queue<Event, 2> events;
  EXPECT_TRUE(events.push(Event{1, 1000}).IsOk());
  utl::optional<Event> const event = events.pop();
  ASSERT_TRUE(event.HasValue());
  EXPECT_EQ(event->kind, 1);
  EXPECT_EQ(event->value, 1000);
}

}  // namespace
}  // namespace utl_test
