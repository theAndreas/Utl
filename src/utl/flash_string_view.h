// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A string_view of text in flash, `PROGMEM` or `F("...")`, which has no
 *        counterpart in the standard.
 */

#ifndef UTL_SRC_UTL_FLASH_STRING_VIEW_H_
#define UTL_SRC_UTL_FLASH_STRING_VIEW_H_

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <stddef.h>

#include "utl/index_error.h"
#include "utl/misuse.h"
#include "utl/result.h"
#include "utl/string_view.h"

namespace utl {

/*!
 * \brief A view of chars in flash.
 *
 * A pointer into flash and a size, 4 bytes on the Uno. On the AVR flash is an
 * address space of its own: a pointer into it read as one into RAM gives
 * whatever lies at that address in RAM. So every char is read with
 * `pgm_read_byte()`, and the view takes the type `F()` gives, so that a
 * pointer into RAM cannot be passed by mistake:
 *
 * \code{.cpp}
 * utl::flash_string_view const keyword{F("text=")};
 * if (line.starts_with(keyword)) { ... }  // line, a string_view in RAM
 * if (line == F("clear")) { ... }
 * \endcode
 *
 * Its names follow string_view, as far as they go: at() and substr() return a
 * Result as there, and starts_with(), ends_with(), find() and `==` work with a
 * text in RAM.
 *
 * \tparam CharT The type of a char, one byte; flash_string_view is the one for
 *               `char`.
 *
 * \note operator[], front() and back() return a char rather than a reference,
 *       as there is nothing in RAM to refer to.
 */
template <typename CharT>
class basic_flash_string_view {
  static_assert(sizeof(CharT) == 1, "flash is read a byte at a time");

 public:
  using value_type = CharT; /*!< The type of a char. */
  using size_type = size_t; /*!< The type of a size. */

  /*! \brief What find() returns when it finds nothing. */
  static constexpr size_type npos{static_cast<size_type>(-1)};

  /*! \brief Walks the chars, reading each from flash. */
  class const_iterator {
   public:
    /*!
     * \brief Makes an iterator at `position`, an address in flash.
     *
     * \param[in] position The address of a char in flash.
     */
    explicit const_iterator(CharT const* position) : position_{position} {}

    /*! \brief Returns the char, read from flash. */
    CharT operator*() const {
      return static_cast<CharT>(pgm_read_byte(position_));
    }

    /*! \brief Moves to the next char, and returns the iterator. */
    const_iterator& operator++() {
      ++position_;
      return *this;
    }

    /*!
     * \brief Returns whether `a` and `b` are at the same char.
     *
     * \param[in] a An iterator.
     * \param[in] b Another iterator.
     */
    friend bool operator==(const_iterator a, const_iterator b) {
      return a.position_ == b.position_;
    }

    /*!
     * \brief Returns whether `a` and `b` are at different chars.
     *
     * \param[in] a An iterator.
     * \param[in] b Another iterator.
     */
    friend bool operator!=(const_iterator a, const_iterator b) {
      return !(a == b);
    }

   private:
    CharT const* position_;
  };

  using iterator = const_iterator; /*!< The same: a view cannot write. */

  /*! \brief Makes an empty view, with no data. */
  constexpr basic_flash_string_view() : data_{nullptr}, size_{0} {}

  /*!
   * \brief Makes a view of the chars of a text in flash up to its `'\0'`.
   *
   * Implicit, so that `F("...")` passes where a flash_string_view is taken.
   *
   * \param[in] text The text, as `F()` gives it.
   */
  basic_flash_string_view(
      __FlashStringHelper const* text)  // NOLINT(runtime/explicit)
      : data_(reinterpret_cast<CharT const*>(text)),
        size_(strlen_P(reinterpret_cast<PGM_P>(text))) {}

  /*!
   * \brief Makes a view of `size` chars of a text in flash.
   *
   * For a `PROGMEM` array:
   * `flash_string_view(reinterpret_cast<const __FlashStringHelper*>(kText),
   * 4)`.
   *
   * \param[in] data The first char, in flash.
   * \param[in] size The number of chars.
   */
  basic_flash_string_view(__FlashStringHelper const* data, size_type size)
      : data_{reinterpret_cast<CharT const*>(data)}, size_{size} {}

  /*!
   * \brief Returns the text in flash, for `Serial.print()`.
   *
   * \warning `Serial.print()` prints up to the `'\0'`, not size() chars.
   */
  __FlashStringHelper const* data() const {
    return reinterpret_cast<__FlashStringHelper const*>(data_);
  }

  /*! \brief Returns the number of chars. */
  constexpr size_type size() const { return size_; }

  /*! \brief Returns the number of chars, as size() does. */
  constexpr size_type length() const { return size_; }

  /*! \brief Returns whether the view has no chars. */
  constexpr bool empty() const { return size_ == 0; }

  /*! \brief Returns an iterator to the first char. */
  const_iterator begin() const { return const_iterator{data_}; }

  /*! \brief Returns an iterator past the last char. */
  const_iterator end() const { return const_iterator{data_ + size_}; }

  /*!
   * \brief Returns the char at `index`, unchecked.
   *
   * \param[in] index The position, from 0.
   * \pre `index` < size(). Beyond the end the read is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kIndex.
   */
  CharT operator[](size_type index) const {
    static_cast<void>(internal::Check(index < size_, Misuse::kIndex));
    return Read(index);
  }

  /*!
   * \brief Returns the first char, unchecked.
   *
   * \pre The view is not empty(). If it is, the read is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kEmpty.
   */
  CharT front() const {
    static_cast<void>(internal::Check(size_ != 0, Misuse::kEmpty));
    return Read(0);
  }

  /*!
   * \brief Returns the last char, unchecked.
   *
   * \pre The view is not empty(), as for front().
   */
  CharT back() const {
    static_cast<void>(internal::Check(size_ != 0, Misuse::kEmpty));
    return Read(size_ - 1);
  }

  /*!
   * \brief Returns the char at `index`, or IndexError::kOutOfRange.
   *
   * \param[in] index The position, from 0.
   */
  Result<CharT> at(size_type index) const {
    if (index >= size_) {
      return IndexError::kOutOfRange;
    }
    return Read(index);
  }

  /*!
   * \brief Returns at most `count` chars from `position` on, as
   *        string_view::substr().
   *
   * \param[in] position The position of the first char, from 0.
   * \param[in] count The most chars to take; all to the end by default.
   * \return The view of them, or IndexError::kOutOfRange if `position` lies
   *         beyond the end.
   */
  Result<basic_flash_string_view> substr(size_type position,
                                         size_type count = npos) const {
    if (position > size_) {
      return IndexError::kOutOfRange;
    }
    size_type const rest{size_ - position};
    return basic_flash_string_view{data_ + position,
                                   count < rest ? count : rest};
  }

  /*!
   * \brief Returns whether the view begins with `prefix`, a text in RAM.
   *
   * \param[in] prefix The text to look for.
   */
  bool starts_with(basic_string_view<CharT> prefix) const {
    return prefix.size() <= size_ &&
           memcmp_P(prefix.data(), data_, prefix.size()) == 0;
  }

  /*!
   * \brief Returns whether the view ends with `suffix`, a text in RAM.
   *
   * \param[in] suffix The text to look for.
   */
  bool ends_with(basic_string_view<CharT> suffix) const {
    return suffix.size() <= size_ &&
           memcmp_P(suffix.data(), data_ + (size_ - suffix.size()),
                    suffix.size()) == 0;
  }

  /*!
   * \brief Returns the position of the first `text`, a text in RAM, from
   *        `position` on.
   *
   * \param[in] text The text to look for.
   * \param[in] position Where to begin, from 0.
   * \return The position, or npos if there is none.
   */
  size_type find(basic_string_view<CharT> text, size_type position = 0) const {
    if (text.size() > size_) {
      return npos;
    }
    for (size_type index{position}; index <= size_ - text.size(); ++index) {
      if (memcmp_P(text.data(), data_ + index, text.size()) == 0) {
        return index;
      }
    }
    return npos;
  }

  /*!
   * \brief Returns the position of the first `c` from `position` on.
   *
   * \param[in] c The char to look for.
   * \param[in] position Where to begin, from 0.
   * \return The position, or npos if there is none.
   */
  size_type find(CharT c, size_type position = 0) const {
    for (size_type index{position}; index < size_; ++index) {
      if (Read(index) == c) {
        return index;
      }
    }
    return npos;
  }

  /*!
   * \brief Returns whether `a` and `b` hold the same chars.
   *
   * \param[in] a A text in flash.
   * \param[in] b Another text in flash.
   */
  friend bool operator==(basic_flash_string_view a, basic_flash_string_view b) {
    if (a.size_ != b.size_) {
      return false;
    }
    for (size_type index{0}; index < a.size_; ++index) {
      if (a.Read(index) != b.Read(index)) {
        return false;
      }
    }
    return true;
  }

  /*!
   * \brief Returns whether `a` and `b` differ.
   *
   * \param[in] a A text in flash.
   * \param[in] b Another text in flash.
   */
  friend bool operator!=(basic_flash_string_view a, basic_flash_string_view b) {
    return !(a == b);
  }

 private:
  // Unchecked, for the members, which check what they read themselves.
  CharT Read(size_type index) const {
    return static_cast<CharT>(pgm_read_byte(data_ + index));
  }

  // For basic_string_view, which compares with a text in flash but cannot
  // include this header, as this one includes it.
  friend class basic_string_view<CharT>;

  basic_flash_string_view(CharT const* data, size_type size)
      : data_{data}, size_{size} {}

  bool Equals(basic_string_view<CharT> text) const {
    return text.size() == size_ && memcmp_P(text.data(), data_, size_) == 0;
  }

  bool IsPrefixOf(basic_string_view<CharT> text) const {
    return size_ <= text.size() && memcmp_P(text.data(), data_, size_) == 0;
  }

  bool IsSuffixOf(basic_string_view<CharT> text) const {
    return size_ <= text.size() &&
           memcmp_P(text.data() + (text.size() - size_), data_, size_) == 0;
  }

  // In flash.
  CharT const* data_;
  size_type size_;
};

/*! \cond */
// The definition of npos, which C++11 wants for a static member that is used;
// see the one of basic_string_view.
template <typename CharT>
constexpr typename basic_flash_string_view<CharT>::size_type
    basic_flash_string_view<CharT>::npos;
/*! \endcond */

/*! \brief A view of `char`s in flash. */
using flash_string_view = basic_flash_string_view<char>;

}  // namespace utl

#endif  // UTL_SRC_UTL_FLASH_STRING_VIEW_H_
