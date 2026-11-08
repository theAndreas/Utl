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
template <typename T, typename... Ts,
          typename = internal::Void<decltype(T{internal::DeclVal<Ts>()})...>>
constexpr array<T, 1 + sizeof...(Ts)> make_array(T first, Ts... rest) {
  /*
   * Arduino compiles with -fpermissive -Wno-error=narrowing, so
   * `{first, rest...}` alone would only warn on make_array(1, 2.5); the test in
   * the template arguments makes it an error, as GCC keeps narrowing ill-formed
   * where it decides whether a template applies.
   */
  return {{first, rest...}};
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
