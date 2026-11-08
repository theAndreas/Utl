// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A queue between an interrupt and the code it interrupts, without
 *        turning interrupts off, which the standard does not know.
 */

#ifndef UTL_SRC_UTL_SPSC_QUEUE_H_
#define UTL_SRC_UTL_SPSC_QUEUE_H_

#include <stddef.h>
#include <stdint.h>

#include "utl/index_error.h"
#include "utl/internal/in_place.h"
#include "utl/internal/type_traits.h"
#include "utl/optional.h"
#include "utl/status.h"

namespace utl {

/*!
 * \brief A queue of at most `N` elements from one producer to one consumer,
 *        such as an interrupt and `loop()`, without turning interrupts off.
 *
 * The elements and two counts of 1 byte, 18 bytes for 16 `uint8_t`. The
 * producer adds with push(), the consumer takes with pop(), in the order
 * added:
 *
 * \code{.cpp}
 * utl::spsc_queue<uint8_t, 16> keys;
 *
 * ISR(PCINT0_vect) { keys.push(ReadKey()); }  // drops a key when full
 *
 * void loop() {
 *   while (utl::optional<uint8_t> const key{keys.pop()}) {
 *     Handle(*key);
 *   }
 * }
 * \endcode
 *
 * Each count is written by one side only, the producer's by push(), the
 * consumer's by pop() and clear(), and read by the other; a byte is read and
 * written at once on the AVR, so neither side has to stop the other. An
 * element is written before the count that hands it over, and read before
 * the count that hands its place back, which `__atomic_signal_fence()` keeps
 * the compiler to; on the AVR it costs no instruction. The counts run on and
 * wrap, and the place of an element is its count masked by `N - 1`: so `N`
 * is a power of two, and every place is used.
 *
 * \warning One producer and one consumer, on one core: an interrupt and the
 *          code it interrupts, or two interrupts that do not interrupt each
 *          other. Two producers, two consumers, or threads on several cores, as
 *          on the host, need more than this.
 *
 * \tparam T The type of the elements, trivially copyable, so that copying one
 *           in an interrupt runs no code of its own; with a default
 *           constructor, as pop() returns an optional.
 * \tparam N The most elements it holds: a power of two, 1 to 128, as the counts
 *           are bytes; checked when compiled.
 */
template <typename T, size_t N>
class spsc_queue {
  static_assert(N >= 1 && N <= 128 && (N & (N - 1)) == 0,
                "spsc_queue: N is a power of two from 1 to 128");
  static_assert(__is_trivially_copyable(T),
                "spsc_queue: T is trivially copyable, so that copying it in "
                "an interrupt runs no code of its own");

 public:
  using value_type = T;     /*!< The type of an element. */
  using size_type = size_t; /*!< The type of a size. */

  /*! \brief Makes an empty queue. */
  // Not = default, which would not be constexpr, as the counts are volatile.
  // NOLINTNEXTLINE(modernize-use-equals-default)
  constexpr spsc_queue() {}

  /*! \brief Deleted: the two sides refer to one queue. */
  spsc_queue(spsc_queue const&) = delete;

  /*! \brief Deleted: the two sides refer to one queue. */
  spsc_queue& operator=(spsc_queue const&) = delete;

  /*!
   * \brief Adds `value` at the end, or returns IndexError::kFull; for the
   *        producer.
   *
   * By value, so that a register passes as it is, `push(UDR0)`, and a small
   * element in registers.
   *
   * \param[in] value The element to add.
   * \return Ok, or IndexError::kFull if it holds `N` elements, which leaves
   *         `value` out.
   */
  Status push(T value) {
    uint8_t const pushed{pushed_};
    if (static_cast<uint8_t>(pushed - popped_) == N) {
      return IndexError::kFull;
    }
    // The place is free once popped_ says so, not before.
    __atomic_signal_fence(__ATOMIC_ACQUIRE);
    ::new (internal::InPlace{&storage_.elements[pushed & kMask]}) T(value);
    // The element is there before the count hands it over.
    __atomic_signal_fence(__ATOMIC_RELEASE);
    pushed_ = static_cast<uint8_t>(pushed + 1);
    return Status::Ok();
  }

  /*!
   * \brief Takes the first element, or returns none if empty; for the consumer.
   *
   * \return The element, or nullopt if there is none.
   */
  optional<T> pop() {
    uint8_t const popped{popped_};
    if (pushed_ == popped) {
      return nullopt;
    }
    // The element is there once pushed_ says so, not before.
    __atomic_signal_fence(__ATOMIC_ACQUIRE);
    T const value{storage_.elements[popped & kMask]};
    // The element is read before the count hands its place back.
    __atomic_signal_fence(__ATOMIC_RELEASE);
    popped_ = static_cast<uint8_t>(popped + 1);
    return value;
  }

  /*!
   * \brief Returns a copy of the first element, or none if empty, and leaves it
   *        in; for the consumer.
   *
   * \return The element, or nullopt if there is none.
   */
  optional<T> front() const {
    uint8_t const popped{popped_};
    if (pushed_ == popped) {
      return nullopt;
    }
    __atomic_signal_fence(__ATOMIC_ACQUIRE);
    return T(storage_.elements[popped & kMask]);
  }

  /*!
   * \brief Removes every element; for the consumer.
   *
   * What the producer adds meanwhile may stay.
   */
  void clear() {
    __atomic_signal_fence(__ATOMIC_RELEASE);
    popped_ = pushed_;
  }

  /*!
   * \brief Returns the number of elements.
   *
   * The other side may change it at once: for the producer, there are at
   * most as many, for the consumer at least.
   */
  size_type size() const { return static_cast<uint8_t>(pushed_ - popped_); }

  /*! \brief Returns whether it holds no element, as size() says. */
  bool empty() const { return pushed_ == popped_; }

  /*! \brief Returns whether it holds `N` elements, as size() says. */
  bool full() const { return size() == N; }

  /*! \brief Returns the most elements it holds, `N`. */
  static constexpr size_type capacity() { return N; }

 private:
  static constexpr uint8_t kMask{N - 1};

  // Room for N elements, none of them made: a union, so that a constexpr
  // constructor makes it by its byte alone, and T needs no constructor here.
  union Storage {
    constexpr Storage() : none{} {}

    uint8_t none;
    T elements[N];  // NOLINT(modernize-avoid-c-arrays)
  };

  Storage storage_{};
  // How many were pushed and popped, each written by one side only, and
  // wrapping: their difference is the number held.
  uint8_t volatile pushed_{0};
  uint8_t volatile popped_{0};
};

}  // namespace utl

#endif  // UTL_SRC_UTL_SPSC_QUEUE_H_
