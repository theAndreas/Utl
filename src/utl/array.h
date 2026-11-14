// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief std::array, which avr-gcc lacks, with at() returning a Result.
 */

#ifndef UTL_SRC_UTL_ARRAY_H_
#define UTL_SRC_UTL_ARRAY_H_

#include <stddef.h>

#include "utl/index_error.h"
#include "utl/internal/type_traits.h"
#include "utl/misuse.h"
#include "utl/result.h"

namespace utl {

/*!
 * \brief A C array of N elements that knows its size.
 *
 * It is copied and compared as a whole, and goes through a range-based `for`:
 *
 * \code{.cpp}
 * utl::array<uint8_t, 3> rows{{0x18, 0x3c, 0x18}};
 * for (uint8_t row : rows) { ... }
 * rows.at(5).ValueOr(0);  // 0, as 5 lies beyond the end
 * \endcode
 *
 * It follows std::array and keeps its names, so that code written for
 * std::array compiles with it. A struct with a public member, as std::array
 * is: so it is an aggregate, is initialized as a C array is, and can lie in
 * `PROGMEM` as one.
 *
 * \tparam T The type of the elements.
 * \tparam N The number of elements, at least 1; checked when compiled.
 *
 * \stddiff at() returns a Result with IndexError::kOutOfRange where
 *          std::array::at() throws, as a sketch is built without exceptions. No
 *          array of no elements.
 */
template <typename T, size_t N>
struct array {
  // A C array of no elements is ill-formed, and nothing here needs one.
  static_assert(N > 0, "utl::array needs at least one element");

  using value_type = T;             /*!< The type of an element. */
  using size_type = size_t;         /*!< The type of a size. */
  using reference = T&;             /*!< A reference to an element. */
  using const_reference = T const&; /*!< One that cannot write. */
  using pointer = T*;               /*!< A pointer to an element. */
  using const_pointer = T const*;   /*!< One that cannot write. */
  using iterator = T*;              /*!< An iterator, a pointer. */
  using const_iterator = T const*;  /*!< One that cannot write. */

  /*!
   * \brief The elements.
   *
   * Public, so that the array is an aggregate; use the functions below rather
   * than this member.
   */
  T elements[N];  // NOLINT(modernize-avoid-c-arrays)

  /*! \brief Returns the number of elements, N. */
  constexpr size_type size() const { return N; }

  /*! \brief Returns the number of elements, N, as std::array does. */
  constexpr size_type max_size() const { return N; }

  /*! \brief Returns false: an array has at least one element. */
  constexpr bool empty() const { return false; }

  /*!
   * \brief Returns the element at `index`, unchecked.
   *
   * \param[in] index The position, from 0.
   * \pre `index` < N. Beyond the end the access is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kIndex.
   */
  reference operator[](size_type index) {
    return static_cast<void>(internal::Check(index < N, Misuse::kIndex)),
           elements[index];
  }

  /*! \copydoc operator[](size_type) */
  constexpr const_reference operator[](size_type index) const {
    return static_cast<void>(internal::Check(index < N, Misuse::kIndex)),
           elements[index];
  }

  /*!
   * \brief Returns the element at `index`, or IndexError::kOutOfRange.
   *
   * A copy, as a Result holds a value. To write, check the index and use
   * operator[].
   *
   * \param[in] index The position, from 0.
   * \return The element, or IndexError::kOutOfRange if `index` >= N.
   *
   * \stddiff Returns a Result where std::array::at() throws std::out_of_range.
   */
  Result<internal::RemoveCv<T>> at(size_type index) const {
    if (index >= N) {
      return IndexError::kOutOfRange;
    }
    return elements[index];
  }

  /*! \brief Returns the first element. */
  reference front() { return elements[0]; }

  /*! \copydoc front() */
  constexpr const_reference front() const { return elements[0]; }

  /*! \brief Returns the last element. */
  reference back() { return elements[N - 1]; }

  /*! \copydoc back() */
  constexpr const_reference back() const { return elements[N - 1]; }

  /*! \brief Returns the address of the first element. */
  pointer data() { return elements; }

  /*! \copydoc data() */
  constexpr const_pointer data() const { return elements; }

  /*! \brief Returns an iterator to the first element. */
  iterator begin() { return elements; }

  /*! \copydoc begin() */
  constexpr const_iterator begin() const { return elements; }

  /*! \brief Returns an iterator to the first element that cannot write. */
  constexpr const_iterator cbegin() const { return elements; }

  /*! \brief Returns an iterator past the last element. */
  iterator end() { return elements + N; }

  /*! \copydoc end() */
  constexpr const_iterator end() const { return elements + N; }

  /*! \brief Returns an iterator past the last element that cannot write. */
  constexpr const_iterator cend() const { return elements + N; }

  /*!
   * \brief Sets every element to `value`.
   *
   * \param[in] value The value to set.
   */
  void fill(T const& value) {
    for (T& element : elements) {
      element = value;
    }
  }
};

/*!
 * \brief Returns an array of its arguments, with the type and the size taken
 *        from them.
 *
 * What `std::array a = {...}` deduces from C++17 on, and gnu++11 cannot. As
 * `std::experimental::make_array`, the type is that of the first argument,
 * after an array decays to a pointer; the others have to convert to it without
 * narrowing, checked when compiled.
 *
 * \code{.cpp}
 * constexpr auto kPins = utl::make_array(uint8_t{2}, uint8_t{3});
 * constexpr auto kMessages PROGMEM = utl::make_array(kNackText, kTimeoutText);
 * \endcode
 *
 * \note A table in `PROGMEM` has to be `constexpr`, so that it is complete when
 *       compiled: initialized at start-up, it would be written to flash, which
 *       fails without a word.
 *
 * \param[in] first The first element, which gives the type.
 * \param[in] rest The other elements.
 */
template <typename T, typename... Ts, typename = internal::BracedFrom<T, Ts...>>
constexpr array<T, 1 + sizeof...(Ts)> make_array(T first, Ts... rest) {
  /*
   * Arduino compiles with -fpermissive -Wno-error=narrowing, so
   * `{first, rest...}` alone would only warn on make_array(1, 2.5); the test in
   * the template arguments makes it an error, as GCC keeps narrowing ill-formed
   * where it decides whether a template applies.
   */
  return {{first, rest...}};
}

namespace internal {

// Declared only: make_array<Element>() calls it for a value that does not fit,
// and a call of a function that is not constexpr ends the evaluation when
// compiling with an error that names it.
template <typename Element>
Element ValueOutOfRange();

// Whether `value` is below 0, without comparing a type that has no sign with 0,
// which -Wtype-limits reports. Not SignOf of internal/charconv.h, which would
// bring that header's error group and its own findings into every array.
template <typename Value>
constexpr bool IsNegative(Value value) {
  return value != Value{0} && !(value > Value{0});
}

// An element of make_array<Element>() made of a value that would narrow, a
// number, which has to come back from the element as it was, sign included: -1
// is 255 as a uint8_t, and 255 back is no -1.
template <typename Element, typename Value, typename = void>
struct ArrayElement {
  static constexpr bool Fits(Value value) {
    return static_cast<Value>(static_cast<Element>(value)) == value &&
           IsNegative(value) == IsNegative(static_cast<Element>(value));
  }

  static constexpr Element Of(Value value) {
    return Fits(value) ? static_cast<Element>(value)
                       : ValueOutOfRange<Element>();
  }
};

// A value that becomes an Element without narrowing is taken as it is: an
// Element already, a smaller number, or a char* for a char const*, which the
// check above could not cast back.
template <typename Element, typename Value>
struct ArrayElement<Element, Value, BracedFrom<Element, Value>> {
  static constexpr Element Of(Value const& value) { return value; }
};

}  // namespace internal

/*!
 * \brief Returns an array of `Element` of its arguments, with the size taken
 *        from them.
 *
 * For a table whose values are written as numbers, as a font or a bitmap: a
 * literal such as `0x3C` is an `int`, which make_array() above would take as
 * the type, or refuse after a first `uint8_t` as narrowing.
 *
 * \code{.cpp}
 * constexpr auto kHeart PROGMEM =
 *     utl::make_array<uint8_t>(0x0C, 0x1E, 0x3C, 0x1E, 0x0C);
 * \endcode
 *
 * A value that does not fit `Element`, as 256 or -1 for a `uint8_t`, stops
 * the build with an error that names `ValueOutOfRange`, also in a sketch,
 * which Arduino compiles with `-Wno-error=narrowing`, so that braces would only
 * warn. Where the first argument is an `Element` already and the others
 * convert to one without narrowing, C++ picks make_array() above as the more
 * specialized, which makes the same array.
 *
 * \note For values known when compiling, as a table in `PROGMEM` has to be
 *       made: initialized at start-up, it would be written to flash, which
 *       fails without a word.
 *
 * \tparam Element The type of the elements, which cannot be left out.
 * \param[in] values The elements, each what becomes an `Element` without
 *                   narrowing, as a `char*` for a `char const*`, or a number
 *                   that fits one.
 *
 * \stddiff `std::experimental::make_array<D>()` refuses a number of another
 *          type even where it fits, so that every value of a font would have to
 *          be written as a `uint8_t`.
 */
template <typename Element, typename... Values>
constexpr array<Element, sizeof...(Values)> make_array(Values... values) {
  return {{internal::ArrayElement<Element, Values>::Of(values)...}};
}

/*!
 * \brief Returns whether `a` and `b` hold equal elements, one by one.
 *
 * \param[in] a An array.
 * \param[in] b Another array of the same type and size.
 */
template <typename T, size_t N>
bool operator==(array<T, N> const& a, array<T, N> const& b) {
  for (size_t index{0}; index < N; ++index) {
    if (!(a[index] == b[index])) {
      return false;
    }
  }
  return true;
}

/*!
 * \brief Returns whether `a` and `b` differ in an element.
 *
 * \param[in] a An array.
 * \param[in] b Another array of the same type and size.
 */
template <typename T, size_t N>
bool operator!=(array<T, N> const& a, array<T, N> const& b) {
  return !(a == b);
}

}  // namespace utl

#endif  // UTL_SRC_UTL_ARRAY_H_
