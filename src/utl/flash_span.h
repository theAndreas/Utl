// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A span of elements in flash, `PROGMEM`, which has no counterpart in
 *        the standard.
 */

#ifndef UTL_SRC_UTL_FLASH_SPAN_H_
#define UTL_SRC_UTL_FLASH_SPAN_H_

#include <avr/pgmspace.h>
#include <stddef.h>
#include <stdint.h>

#include "utl/array.h"
#include "utl/index_error.h"
#include "utl/internal/type_traits.h"
#include "utl/misuse.h"
#include "utl/result.h"

namespace utl {

/*! \cond */
// What a sketch does not see: how an element is read from flash.
namespace internal {

// Reads the T at `from`, in flash, into `to`, in RAM: one of 1, 2 or 4 bytes
// with one read of its width, any other with memcpy_P(). The bytes go through
// __builtin_memcpy, as T is trivially copyable and needs no constructor.
template <typename T, size_t Size = sizeof(T)>
struct FlashRead {
  static void Into(T& to, T const* from) { memcpy_P(&to, from, Size); }
};
template <typename T>
struct FlashRead<T, 1> {
  static void Into(T& to, T const* from) {
    uint8_t const value{pgm_read_byte(from)};
    __builtin_memcpy(&to, &value, 1);
  }
};
template <typename T>
struct FlashRead<T, 2> {
  static void Into(T& to, T const* from) {
    uint16_t const value{pgm_read_word(from)};
    __builtin_memcpy(&to, &value, 2);
  }
};
template <typename T>
struct FlashRead<T, 4> {
  static void Into(T& to, T const* from) {
    uint32_t const value{pgm_read_dword(from)};
    __builtin_memcpy(&to, &value, 4);
  }
};

/*
 * Returns the element at `from`, in flash, as a T in RAM. Through a union, as
 * T needs no default constructor: trivially copyable, it is its bytes.
 */
template <typename T>
T ReadFlash(T const* from) {
  union Bytes {
    constexpr Bytes() : none{} {}
    uint8_t none;
    T value;
  } bytes;
  FlashRead<T>::Into(bytes.value, from);
  return bytes.value;
}

}  // namespace internal
/*! \endcond */

/*!
 * \brief A view of elements of `T` in flash, one after the other.
 *
 * A pointer into flash and a size, 4 bytes on the Uno. On the AVR flash is an
 * address space of its own: a pointer into it read as one into RAM gives
 * whatever lies at that address in RAM. So every element is read with
 * `pgm_read_byte()`, `pgm_read_word()` or `pgm_read_dword()` by its size, or
 * `memcpy_P()`, and given as a value, not a reference, as there is nothing in
 * RAM to refer to. It is for a table that never changes, a font or a pattern,
 * which lies in flash rather than in RAM:
 *
 * \code{.cpp}
 * constexpr uint8_t kSmiley[] PROGMEM{0x3c, 0x42, 0xa5, 0x81};
 *
 * utl::flash_span<uint8_t> const smiley(kSmiley);
 * for (uint8_t const row : smiley) { ... }  // each read from flash
 * smiley.at(9).ValueOr(0);                  // 0, as 9 lies beyond the end
 * \endcode
 *
 * Its names follow span, as far as they go: at(), first(), last() and
 * subspan() return a Result as there.
 *
 * \tparam T The type of the elements, trivially copyable, as an element is read
 *           as its bytes; not `const`, as a view of flash cannot write anyway.
 *
 * \warning It is made explicitly of an array, as the type of the array does not
 *          tell whether it lies in flash. One of an array in RAM reads whatever
 *          lies in flash at that address.
 */
template <typename T>
class flash_span {
  static_assert(__is_trivially_copyable(T),
                "flash_span: T is trivially copyable, read as its bytes");
  static_assert(!internal::IsConst<T>::kValue,
                "flash_span: T is not const, as flash cannot be written "
                "anyway; flash_span<uint8_t>, not flash_span<uint8_t const>");

 public:
  using element_type = T;   /*!< The type of an element. */
  using value_type = T;     /*!< The type of an element. */
  using size_type = size_t; /*!< The type of a size. */

  /*! \brief Walks the elements, reading each from flash. */
  class const_iterator {
   public:
    /*!
     * \brief Makes an iterator at `position`, an address in flash.
     *
     * \param[in] position The address of an element in flash.
     */
    explicit const_iterator(T const* position) : position_{position} {}

    /*! \brief Returns the element, read from flash. */
    T operator*() const { return internal::ReadFlash(position_); }

    /*! \brief Moves to the next element, and returns the iterator. */
    const_iterator& operator++() {
      ++position_;
      return *this;
    }

    /*!
     * \brief Returns whether `a` and `b` are at the same element.
     *
     * \param[in] a An iterator.
     * \param[in] b Another iterator.
     */
    friend bool operator==(const_iterator a, const_iterator b) {
      return a.position_ == b.position_;
    }

    /*!
     * \brief Returns whether `a` and `b` are at different elements.
     *
     * \param[in] a An iterator.
     * \param[in] b Another iterator.
     */
    friend bool operator!=(const_iterator a, const_iterator b) {
      return !(a == b);
    }

   private:
    T const* position_;
  };

  using iterator = const_iterator; /*!< The same: a view cannot write. */

  /*! \brief Makes an empty view, with no data. */
  constexpr flash_span() : data_{nullptr}, size_{0} {}

  /*!
   * \brief Makes a view of `size` elements in flash from `data` on.
   *
   * \param[in] data The first element, in flash.
   * \param[in] size The number of elements.
   */
  constexpr flash_span(T const* data, size_type size)
      : data_{data}, size_{size} {}

  /*!
   * \brief Makes a view of a whole C array in flash, `PROGMEM`.
   *
   * \param[in] elements The array, in flash.
   */
  template <size_t N>
  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
  constexpr explicit flash_span(T const (&elements)[N])
      : data_{elements}, size_{N} {}

  /*!
   * \brief Makes a view of a whole utl::array in flash, `PROGMEM`.
   *
   * \param[in] elements The array, in flash.
   */
  template <size_t N>
  constexpr explicit flash_span(array<T, N> const& elements)
      : data_{elements.elements}, size_{N} {}

  /*!
   * \brief Returns the address of the first element, in flash, or null if made
   *        empty.
   *
   * \warning An address in flash: read through it with `pgm_read_byte()` or
   *          `memcpy_P()`, never with `*`.
   */
  constexpr T const* data() const { return data_; }

  /*! \brief Returns the number of elements. */
  constexpr size_type size() const { return size_; }

  /*! \brief Returns the number of bytes the elements take. */
  constexpr size_type size_bytes() const { return size_ * sizeof(T); }

  /*! \brief Returns whether the view has no elements. */
  constexpr bool empty() const { return size_ == 0; }

  /*! \brief Returns an iterator to the first element. */
  const_iterator begin() const { return const_iterator{data_}; }

  /*! \brief Returns an iterator past the last element. */
  const_iterator end() const { return const_iterator{data_ + size_}; }

  /*!
   * \brief Returns the element at `index`, unchecked.
   *
   * \param[in] index The position, from 0.
   * \return The element, read from flash.
   * \pre `index` < size(). Beyond the end the read is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kIndex.
   */
  T operator[](size_type index) const {
    static_cast<void>(internal::Check(index < size_, Misuse::kIndex));
    return internal::ReadFlash(data_ + index);
  }

  /*!
   * \brief Returns the element at `index`, or IndexError::kOutOfRange.
   *
   * \param[in] index The position, from 0.
   * \return The element, or IndexError::kOutOfRange if `index` >= size().
   */
  Result<T> at(size_type index) const {
    if (index >= size_) {
      return IndexError::kOutOfRange;
    }
    return internal::ReadFlash(data_ + index);
  }

  /*!
   * \brief Returns the first element, unchecked.
   *
   * \return The first element, read from flash.
   * \pre The view is not empty(). If it is, the read is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kEmpty.
   */
  T front() const {
    static_cast<void>(internal::Check(size_ != 0, Misuse::kEmpty));
    return internal::ReadFlash(data_);
  }

  /*!
   * \brief Returns the last element, unchecked.
   *
   * \return The last element, read from flash.
   * \pre The view is not empty(), as for front().
   */
  T back() const {
    static_cast<void>(internal::Check(size_ != 0, Misuse::kEmpty));
    return internal::ReadFlash(data_ + (size_ - 1));
  }

  /*!
   * \brief Returns the first `count` elements.
   *
   * \param[in] count The number of elements.
   * \return The view of them, or IndexError::kOutOfRange if there are fewer.
   */
  Result<flash_span> first(size_type count) const {
    if (count > size_) {
      return IndexError::kOutOfRange;
    }
    return flash_span{data_, count};
  }

  /*!
   * \brief Returns the last `count` elements.
   *
   * \param[in] count The number of elements.
   * \return The view of them, or IndexError::kOutOfRange if there are fewer.
   */
  Result<flash_span> last(size_type count) const {
    if (count > size_) {
      return IndexError::kOutOfRange;
    }
    return flash_span{data_ + (size_ - count), count};
  }

  /*!
   * \brief Returns the elements from `offset` to the end.
   *
   * An `offset` of size() gives an empty view, as span::subspan() does.
   *
   * \param[in] offset The position of the first element, from 0.
   * \return The view of them, or IndexError::kOutOfRange if `offset` lies
   *         beyond the end.
   */
  Result<flash_span> subspan(size_type offset) const {
    if (offset > size_) {
      return IndexError::kOutOfRange;
    }
    return flash_span{data_ + offset, size_ - offset};
  }

  /*!
   * \brief Returns `count` elements from `offset` on.
   *
   * \param[in] offset The position of the first element, from 0.
   * \param[in] count The number of elements.
   * \return The view of them, or IndexError::kOutOfRange if they do not all lie
   *         within this view.
   */
  Result<flash_span> subspan(size_type offset, size_type count) const {
    // Written so that offset + count cannot overflow.
    if (offset > size_ || count > size_ - offset) {
      return IndexError::kOutOfRange;
    }
    return flash_span{data_ + offset, count};
  }

 private:
  T const* data_;
  size_type size_;
};

}  // namespace utl

#endif  // UTL_SRC_UTL_FLASH_SPAN_H_
