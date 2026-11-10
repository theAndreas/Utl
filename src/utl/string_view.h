// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief std::string_view, from C++17, for gnu++11, for text in RAM.
 *
 * Text in flash is a flash_string_view.
 */

#ifndef UTL_SRC_UTL_STRING_VIEW_H_
#define UTL_SRC_UTL_STRING_VIEW_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "utl/index_error.h"
#include "utl/misuse.h"
#include "utl/result.h"

namespace utl {

/*! \cond */
template <typename CharT>
class basic_flash_string_view;
/*! \endcond */

/*!
 * \brief A view of chars in RAM.
 *
 * A pointer and a size, 4 bytes on the Uno. It owns nothing: what it views
 * has to outlive it. A typed line, a buffer or a string literal:
 *
 * \code{.cpp}
 * utl::string_view const line(buffer, length);
 * if (line.starts_with("text=")) {
 *   line.substr(5).Map(SetText);  // a Result: substr() can fail
 * }
 * \endcode
 *
 * It follows std::string_view and keeps its names. starts_with(), ends_with()
 * and `==` work with a text in flash as well, as `line == F("clear")`.
 *
 * \tparam CharT The type of a char; string_view is the one for `char`.
 *
 * \stddiff at() and substr() beyond the end return a Result with
 *          IndexError::kOutOfRange where std::string_view throws; find()
 *          returns npos, as "not found" is no error. compare() returns an
 *          `int8_t`, not an `int`.
 */
template <typename CharT>
class basic_string_view {
 public:
  using value_type = CharT;             /*!< The type of a char. */
  using size_type = size_t;             /*!< The type of a size. */
  using const_reference = CharT const&; /*!< A reference to a char. */
  using const_pointer = CharT const*;   /*!< A pointer to a char. */
  using const_iterator = CharT const*;  /*!< An iterator, a pointer. */
  using iterator = const_iterator;      /*!< The same: a view cannot write. */

  /*! \brief What find() returns when it finds nothing. */
  static constexpr size_type npos{static_cast<size_type>(-1)};

  /*! \brief Makes an empty view, with no data. */
  constexpr basic_string_view() : data_{nullptr}, size_{0} {}

  /*!
   * \brief Makes a view of `size` chars from `data` on.
   *
   * \param[in] data The first char.
   * \param[in] size The number of chars.
   */
  constexpr basic_string_view(CharT const* data, size_type size)
      : data_{data}, size_{size} {}

  /*!
   * \brief Makes a view of the chars of a string up to its terminating `'\0'`.
   *
   * Implicit, as in the standard, so that a function that takes a view takes a
   * string literal as it is.
   *
   * \param[in] text The string, terminated by `'\0'`.
   *
   * \stddiff Not `constexpr`.
   */
  basic_string_view(CharT const* text)  // NOLINT(runtime/explicit)
      : data_{text}, size_{strlen(text)} {
    // Not constexpr: a constexpr length in C++11 would be a recursion, one
    // call per char at run time, on a stack of 2 KB.
  }

  /*! \brief Returns the address of the first char. */
  constexpr const_pointer data() const { return data_; }

  /*! \brief Returns the number of chars. */
  constexpr size_type size() const { return size_; }

  /*! \brief Returns the number of chars, as size() does. */
  constexpr size_type length() const { return size_; }

  /*! \brief Returns whether the view has no chars. */
  constexpr bool empty() const { return size_ == 0; }

  /*! \brief Returns an iterator to the first char. */
  constexpr const_iterator begin() const { return data_; }

  /*! \brief Returns an iterator past the last char. */
  constexpr const_iterator end() const { return data_ + size_; }

  /*!
   * \brief Returns the char at `index`, unchecked.
   *
   * \param[in] index The position, from 0.
   * \pre `index` < size(). Beyond the end the access is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kIndex.
   */
  constexpr const_reference operator[](size_type index) const {
    return static_cast<void>(internal::Check(index < size_, Misuse::kIndex)),
           data_[index];
  }

  /*!
   * \brief Returns the first char, unchecked.
   *
   * \pre The view is not empty(). If it is, the access is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kEmpty.
   */
  constexpr const_reference front() const {
    return static_cast<void>(internal::Check(size_ != 0, Misuse::kEmpty)),
           data_[0];
  }

  /*!
   * \brief Returns the last char, unchecked.
   *
   * \pre The view is not empty(), as for front().
   */
  constexpr const_reference back() const {
    return static_cast<void>(internal::Check(size_ != 0, Misuse::kEmpty)),
           data_[size_ - 1];
  }

  /*!
   * \brief Returns the char at `index`, or IndexError::kOutOfRange.
   *
   * \param[in] index The position, from 0.
   *
   * \stddiff Returns a Result where std::string_view::at() throws.
   */
  Result<CharT> at(size_type index) const {
    if (index >= size_) {
      return IndexError::kOutOfRange;
    }
    return data_[index];
  }

  /*!
   * \brief Returns at most `count` chars from `position` on.
   *
   * A `position` of size() gives an empty view, as in the standard.
   *
   * \param[in] position The position of the first char, from 0.
   * \param[in] count The most chars to take; all to the end by default.
   * \return The view of them, or IndexError::kOutOfRange if `position` lies
   *         beyond the end.
   *
   * \stddiff Returns a Result where std::string_view::substr() throws.
   */
  Result<basic_string_view> substr(size_type position,
                                   size_type count = npos) const {
    if (position > size_) {
      return IndexError::kOutOfRange;
    }
    size_type const rest{size_ - position};
    return basic_string_view{data_ + position, count < rest ? count : rest};
  }

  /*!
   * \brief Returns whether the view begins with `prefix`.
   *
   * \param[in] prefix The text to look for, in RAM.
   */
  bool starts_with(basic_string_view prefix) const {
    return prefix.size_ <= size_ && Equal(data_, prefix.data_, prefix.size_);
  }

  /*!
   * \brief Returns whether the view begins with `prefix`, a text in flash.
   *
   * \param[in] prefix The text to look for, as `F("text=")`.
   */
  bool starts_with(basic_flash_string_view<CharT> prefix) const {
    // flash_string_view.h does the work, reading it with pgm_read: a pointer
    // into flash read as one into RAM gives whatever lies there in RAM.
    return prefix.IsPrefixOf(*this);
  }

  /*!
   * \brief Returns whether the view ends with `suffix`.
   *
   * \param[in] suffix The text to look for, in RAM.
   */
  bool ends_with(basic_string_view suffix) const {
    return suffix.size_ <= size_ &&
           Equal(data_ + (size_ - suffix.size_), suffix.data_, suffix.size_);
  }

  /*!
   * \brief Returns whether the view ends with `suffix`, a text in flash.
   *
   * \param[in] suffix The text to look for, as `F("Hi")`.
   */
  bool ends_with(basic_flash_string_view<CharT> suffix) const {
    return suffix.IsSuffixOf(*this);
  }

  /*!
   * \brief Drops the first `count` chars.
   *
   * \param[in] count The number of chars to drop.
   * \pre `count` <= size(). More is reported with `UTL_CHECK_ACCESS` at 1, as
   *      Misuse::kRemove, and leaves an empty view either way.
   *
   * \stddiff Leaves an empty view where the standard leaves more than size()
   *          undefined.
   */
  void remove_prefix(size_type count) {
    static_cast<void>(internal::Check(count <= size_, Misuse::kRemove));
    size_type const removed{count < size_ ? count : size_};
    data_ += removed;
    size_ -= removed;
  }

  /*!
   * \brief Drops the last `count` chars.
   *
   * \param[in] count The number of chars to drop.
   * \pre `count` <= size(), as for remove_prefix().
   */
  void remove_suffix(size_type count) {
    static_cast<void>(internal::Check(count <= size_, Misuse::kRemove));
    size_ -= count < size_ ? count : size_;
  }

  /*!
   * \brief Returns -1, 0 or 1 as this view lies before, at or after `other`.
   *
   * Char by char, each as an `unsigned char`, as in the standard; a prefix
   * lies before the longer text.
   *
   * \param[in] other The view to compare with.
   *
   * \stddiff Returns an `int8_t`, where the standard returns an `int`: only the
   *          sign means anything.
   */
  int8_t compare(basic_string_view other) const {
    size_type const common{size_ < other.size_ ? size_ : other.size_};
    for (size_type index{0}; index < common; ++index) {
      auto const a = static_cast<uint8_t>(data_[index]);
      auto const b = static_cast<uint8_t>(other.data_[index]);
      if (a != b) {
        return a < b ? -1 : 1;
      }
    }
    if (size_ == other.size_) {
      return 0;
    }
    return size_ < other.size_ ? -1 : 1;
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
      if (data_[index] == c) {
        return index;
      }
    }
    return npos;
  }

  /*!
   * \brief Returns the position of the first `text` from `position` on.
   *
   * \param[in] text The text to look for.
   * \param[in] position Where to begin, from 0.
   * \return The position, or npos if there is none.
   */
  size_type find(basic_string_view text, size_type position = 0) const {
    if (text.size_ > size_) {
      return npos;
    }
    for (size_type index{position}; index <= size_ - text.size_; ++index) {
      if (Equal(data_ + index, text.data_, text.size_)) {
        return index;
      }
    }
    return npos;
  }

  /*!
   * \brief Returns whether `a` and `b` hold the same chars.
   *
   * A string literal converts on either side, as in `line == "clear"`.
   *
   * \param[in] a A view.
   * \param[in] b Another view.
   */
  friend bool operator==(basic_string_view a, basic_string_view b) {
    return a.size_ == b.size_ && Equal(a.data_, b.data_, a.size_);
  }

  /*!
   * \brief Returns whether `a` and `b` differ.
   *
   * \param[in] a A view.
   * \param[in] b Another view.
   */
  friend bool operator!=(basic_string_view a, basic_string_view b) {
    return !(a == b);
  }

  /*!
   * \brief Returns whether `a` lies before `b`, as compare() orders.
   *
   * \param[in] a A view.
   * \param[in] b Another view.
   */
  friend bool operator<(basic_string_view a, basic_string_view b) {
    return a.compare(b) < 0;
  }

  /*!
   * \brief Returns whether `a` lies before `b` or at it.
   *
   * \param[in] a A view.
   * \param[in] b Another view.
   */
  friend bool operator<=(basic_string_view a, basic_string_view b) {
    return a.compare(b) <= 0;
  }

  /*!
   * \brief Returns whether `a` lies after `b`.
   *
   * \param[in] a A view.
   * \param[in] b Another view.
   */
  friend bool operator>(basic_string_view a, basic_string_view b) {
    return a.compare(b) > 0;
  }

  /*!
   * \brief Returns whether `a` lies after `b` or at it.
   *
   * \param[in] a A view.
   * \param[in] b Another view.
   */
  friend bool operator>=(basic_string_view a, basic_string_view b) {
    return a.compare(b) >= 0;
  }

  /*!
   * \brief Returns whether `a` holds the chars of `b`, a text in flash.
   *
   * \param[in] a A view in RAM.
   * \param[in] b A text in flash, as `F("clear")`.
   */
  friend bool operator==(basic_string_view a,
                         basic_flash_string_view<CharT> b) {
    return EqualsFlash(a, b);
  }

  /*!
   * \brief Returns whether `b` holds the chars of `a`, a text in flash.
   *
   * \param[in] a A text in flash.
   * \param[in] b A view in RAM.
   */
  friend bool operator==(basic_flash_string_view<CharT> a,
                         basic_string_view b) {
    return EqualsFlash(b, a);
  }

  /*!
   * \brief Returns whether `a` differs from `b`, a text in flash.
   *
   * \param[in] a A view in RAM.
   * \param[in] b A text in flash.
   */
  friend bool operator!=(basic_string_view a,
                         basic_flash_string_view<CharT> b) {
    return !EqualsFlash(a, b);
  }

  /*!
   * \brief Returns whether `b` differs from `a`, a text in flash.
   *
   * \param[in] a A text in flash.
   * \param[in] b A view in RAM.
   */
  friend bool operator!=(basic_flash_string_view<CharT> a,
                         basic_string_view b) {
    return !EqualsFlash(b, a);
  }

 private:
  // A member, unlike the operators above, so that the friendship of
  // basic_flash_string_view reaches it.
  static bool EqualsFlash(basic_string_view a,
                          basic_flash_string_view<CharT> b) {
    return b.Equals(a);
  }

  static bool Equal(CharT const* a, CharT const* b, size_type count) {
    for (size_type index{0}; index < count; ++index) {
      if (a[index] != b[index]) {
        return false;
      }
    }
    return true;
  }

  CharT const* data_;
  size_type size_;
};

/*! \cond */
/*
 * The definition of npos, which C++11 wants for a static member that is used.
 * The class is a template, as std::basic_string_view, so that it can lie in
 * this header: C++11 has no inline variables, and the static member of a plain
 * class would be defined once per file that includes it.
 */
template <typename CharT>
constexpr
    typename basic_string_view<CharT>::size_type basic_string_view<CharT>::npos;
/*! \endcond */

/*! \brief A view of `char`s in RAM. */
using string_view = basic_string_view<char>;

}  // namespace utl

#endif  // UTL_SRC_UTL_STRING_VIEW_H_
