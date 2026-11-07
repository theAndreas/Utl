// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief std::span, from C++20, for gnu++11.
 *
 * What can go out of range returns a Result instead of being undefined.
 */

#ifndef UTL_SRC_UTL_SPAN_H_
#define UTL_SRC_UTL_SPAN_H_

#include <stddef.h>

#include "utl/array.h"
#include "utl/index_error.h"
#include "utl/internal/type_traits.h"
#include "utl/misuse.h"
#include "utl/result.h"

namespace utl {

/*! \cond */
// Declared only: a span is made of one, which includes this header.
template <typename T, size_t N>
class inplace_vector;
/*! \endcond */

/*!
 * \brief A view of elements that lie one after the other.
 *
 * A pointer and a size, 4 bytes on the Uno. A span owns nothing: whatever it
 * views has to outlive it. A function that takes a span takes a C array, a
 * utl::array, a utl::inplace_vector, another span, or a pointer and a size
 * alike:
 *
 * \code{.cpp}
 * uint8_t Sum(utl::span<uint8_t const> rows);
 *
 * utl::array<uint8_t, 8> rows = {};
 * Sum(rows);
 * Sum(utl::span<uint8_t const>(buffer, length));
 * \endcode
 *
 * It follows std::span and keeps its names, as the Google style allows for an
 * entity that mimics the standard library.
 *
 * \tparam T The type of the elements, `const` for a view that cannot write.
 *
 * \stddiff Only a dynamic extent. at(), first(), last() and subspan() return a
 *          Result with IndexError::kOutOfRange where std::span leaves the
 *          access undefined or throws. operator[], front() and back() stay
 *          unchecked, unless `UTL_CHECK_ACCESS` is 1.
 */
template <typename T>
class span {
 public:
  using element_type = T;                   /*!< The type of an element. */
  using value_type = internal::RemoveCv<T>; /*!< T without `const`. */
  using size_type = size_t;                 /*!< The type of a size. */
  using reference = T&;                     /*!< A reference to an element. */
  using pointer = T*;                       /*!< A pointer to an element. */
  using iterator = T*;                      /*!< An iterator, a pointer. */

  /*! \brief Makes an empty span, with no data. */
  constexpr span() : data_(nullptr), size_(0) {}

  /*!
   * \brief Makes a span of `size` elements from `data` on.
   *
   * \param[in] data The first element.
   * \param[in] size The number of elements.
   */
  constexpr span(T* data, size_type size) : data_(data), size_(size) {}

  /*
   * The conversions below are implicit, as in std::span: a span is a view of
   * what it is made from, and a function that takes one should take a C array
   * or a utl::array as it is. Each takes part only if an array of the source
   * may be seen as an array of T, so a span of Derived never becomes a span
   * of Base.
   */

  /*!
   * \brief Makes a span of a whole C array.
   *
   * \param[in] elements The array.
   */
  template <typename U, size_t N, typename = internal::ArrayConvertible<U, T>>
  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
  constexpr span(U (&elements)[N])  // NOLINT(runtime/explicit)
      : data_(elements), size_(N) {}

  /*!
   * \brief Makes a span of a whole utl::array.
   *
   * \param[in] elements The array.
   */
  template <typename U, size_t N, typename = internal::ArrayConvertible<U, T>>
  constexpr span(array<U, N>& elements)  // NOLINT(runtime/explicit)
      : data_(elements.data()), size_(N) {}

  /*!
   * \brief Makes a span of `const` of a whole utl::array that is `const`.
   *
   * \param[in] elements The array.
   */
  template <typename U, size_t N,
            typename = internal::ArrayConvertible<U const, T>>
  constexpr span(array<U, N> const& elements)  // NOLINT(runtime/explicit)
      : data_(elements.data()), size_(N) {}

  /*!
   * \brief Makes a span of the elements a utl::inplace_vector holds now.
   *
   * It holds as many as the vector does when made, and does not grow with it.
   *
   * \param[in] vector The vector.
   */
  template <typename U, size_t N, typename = internal::ArrayConvertible<U, T>>
  constexpr span(inplace_vector<U, N>& vector)  // NOLINT(runtime/explicit)
      : data_(vector.data()), size_(vector.size()) {}

  /*!
   * \brief Makes a span of `const` of the elements a utl::inplace_vector that
   *        is `const` holds now.
   *
   * \param[in] vector The vector.
   */
  template <typename U, size_t N,
            typename = internal::ArrayConvertible<U const, T>>
  constexpr span(
      inplace_vector<U, N> const& vector)  // NOLINT(runtime/explicit)
      : data_(vector.data()), size_(vector.size()) {}

  /*!
   * \brief Makes a span of another, as a `span<uint8_t>` into a
   *        `span<uint8_t const>`.
   *
   * \param[in] other The span to view the elements of.
   */
  template <typename U, typename = internal::ArrayConvertible<U, T>>
  constexpr span(span<U> const& other)  // NOLINT(runtime/explicit)
      : data_(other.data()), size_(other.size()) {}

  /*! \brief Returns the first element's address, or null if made empty. */
  constexpr pointer data() const { return data_; }

  /*! \brief Returns the number of elements. */
  constexpr size_type size() const { return size_; }

  /*! \brief Returns the number of bytes the elements take. */
  constexpr size_type size_bytes() const { return size_ * sizeof(T); }

  /*! \brief Returns whether the span has no elements. */
  constexpr bool empty() const { return size_ == 0; }

  /*!
   * \brief Returns the element at `index`, unchecked.
   *
   * \param[in] index The position, from 0.
   * \return The element.
   * \pre `index` < size(). Beyond the end the access is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kIndex.
   */
  constexpr reference operator[](size_type index) const {
    return static_cast<void>(internal::Check(index < size_, Misuse::kIndex)),
           data_[index];
  }

  /*!
   * \brief Returns the element at `index`, or IndexError::kOutOfRange.
   *
   * A copy, as a Result holds a value. To write, check the index and use
   * operator[].
   *
   * \param[in] index The position, from 0.
   * \return The element, or IndexError::kOutOfRange if `index` >= size().
   *
   * \stddiff Returns a Result where std::span::at() throws std::out_of_range.
   */
  Result<value_type> at(size_type index) const {
    if (index >= size_) {
      return IndexError::kOutOfRange;
    }
    return data_[index];
  }

  /*!
   * \brief Returns the first element, unchecked.
   *
   * \return The first element.
   * \pre The span is not empty(). If it is, the access is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kEmpty.
   */
  constexpr reference front() const {
    return static_cast<void>(internal::Check(size_ != 0, Misuse::kEmpty)),
           data_[0];
  }

  /*!
   * \brief Returns the last element, unchecked.
   *
   * \return The last element.
   * \pre The span is not empty(), as for front().
   */
  constexpr reference back() const {
    return static_cast<void>(internal::Check(size_ != 0, Misuse::kEmpty)),
           data_[size_ - 1];
  }

  /*! \brief Returns an iterator to the first element. */
  constexpr iterator begin() const { return data_; }

  /*! \brief Returns an iterator past the last element. */
  constexpr iterator end() const { return data_ + size_; }

  /*!
   * \brief Returns the first `count` elements.
   *
   * \param[in] count The number of elements.
   * \return The span of them, or IndexError::kOutOfRange if there are fewer.
   *
   * \stddiff Returns a Result where std::span::first() is undefined.
   */
  Result<span> first(size_type count) const {
    if (count > size_) {
      return IndexError::kOutOfRange;
    }
    return span(data_, count);
  }

  /*!
   * \brief Returns the last `count` elements.
   *
   * \param[in] count The number of elements.
   * \return The span of them, or IndexError::kOutOfRange if there are fewer.
   *
   * \stddiff Returns a Result where std::span::last() is undefined.
   */
  Result<span> last(size_type count) const {
    if (count > size_) {
      return IndexError::kOutOfRange;
    }
    return span(data_ + (size_ - count), count);
  }

  /*!
   * \brief Returns the elements from `offset` to the end.
   *
   * An `offset` of size() gives an empty span, as in std::span.
   *
   * \param[in] offset The position of the first element, from 0.
   * \return The span of them, or IndexError::kOutOfRange if `offset` lies
   *         beyond the end.
   *
   * \stddiff Returns a Result where std::span::subspan() is undefined.
   */
  Result<span> subspan(size_type offset) const {
    if (offset > size_) {
      return IndexError::kOutOfRange;
    }
    return span(data_ + offset, size_ - offset);
  }

  /*!
   * \brief Returns `count` elements from `offset` on.
   *
   * \param[in] offset The position of the first element, from 0.
   * \param[in] count The number of elements.
   * \return The span of them, or IndexError::kOutOfRange if they do not all lie
   *         within this span.
   *
   * \stddiff Returns a Result where std::span::subspan() is undefined.
   */
  Result<span> subspan(size_type offset, size_type count) const {
    // Written so that offset + count cannot overflow.
    if (offset > size_ || count > size_ - offset) {
      return IndexError::kOutOfRange;
    }
    return span(data_ + offset, count);
  }

 private:
  T* data_;
  size_type size_;
};

}  // namespace utl

#endif  // UTL_SRC_UTL_SPAN_H_
