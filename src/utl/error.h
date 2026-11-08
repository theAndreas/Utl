// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief An error: the group it comes from and a code within that group.
 */

#ifndef UTL_SRC_UTL_ERROR_H_
#define UTL_SRC_UTL_ERROR_H_

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "utl/error_group.h"

namespace utl {

class Status;

/*!
 * \brief An error, always: a group and a code within it.
 *
 * 3 bytes on the Uno. What a function returns that may succeed is a Status or
 * a Result, which hold an Error only when something went wrong; "no error" is
 * neither an Error nor a code that a group would have to keep free.
 *
 * \code{.cpp}
 * utl::Error const error{FontError::kUnknownChar};
 * error.group().Name();  // "font", never null
 * error.Message();       // "unknown char", once UseFontMessages() was called
 * \endcode
 *
 * Two errors are equal if both the group and the code are, so the codes of
 * different groups may overlap. A dropped Error warns, as `[[nodiscard]]`:
 * nothing returns one but to be looked at.
 */
class [[nodiscard]] Error {
 public:
  /*!
   * \brief Makes the error `code` of `group`.
   *
   * \param[in] group The group.
   * \param[in] code The code within the group.
   */
  constexpr Error(ErrorGroup group, uint8_t code)
      : group_{group}, code_{code} {}

  /*!
   * \brief Makes the error of a code of a group, as `FontError::kUnknownChar`.
   *
   * Implicit, so that `return FontError::kUnknownChar;` and
   * `error == FontError::kUnknownChar` need no function around the code. Only
   * an enum with a `GetErrorGroup()` beside it converts; see ErrorGroup.
   *
   * \param[in] code The code; its group is `GetErrorGroup(code)`.
   */
  template <typename Code, typename = decltype(GetErrorGroup(Code{}))>
  constexpr Error(Code code)  // NOLINT(runtime/explicit)
      : group_(GetErrorGroup(code)), code_(static_cast<uint8_t>(code)) {
    /*
     * Implicit, unlike the Google default, because a code of a group is an
     * error and nothing else. The default template argument takes it out of
     * overload resolution for any other type.
     */
  }

  /*! \brief Returns the group the error comes from. */
  ErrorGroup group() const { return group_; }

  /*! \brief Returns the code within the group. */
  uint8_t code() const { return code_; }

  /*!
   * \brief Returns the text of the error, from flash.
   *
   * \return The text, or an empty text if the texts of its group are off or
   *         have none for the code; see UseMessages().
   */
  __FlashStringHelper const* Message() const;

  /*!
   * \brief Prints the group and the text, as `bus: nack`.
   *
   * If the texts of the group are off, it prints the code instead,
   * as `bus: 0`.
   *
   * \param[out] out Where to print, such as `Serial`.
   * \return The number of chars printed, as Print::print() returns.
   */
  size_t PrintTo(Print& out) const;

  /*!
   * \brief Returns whether `a` and `b` are the same error, group and code.
   *
   * A code converts on either side, as in `error == FontError::kNullText`.
   *
   * \param[in] a An error.
   * \param[in] b Another error.
   */
  friend bool operator==(Error const& a, Error const& b) {
    // A hidden friend: found only through an Error, and with the implicit
    // conversion above on either side.
    return a.group_ == b.group_ && a.code_ == b.code_;
  }

  /*!
   * \brief Returns whether `a` and `b` differ in group or code.
   *
   * \param[in] a An error.
   * \param[in] b Another error.
   */
  friend bool operator!=(Error const& a, Error const& b) { return !(a == b); }

 private:
  friend class Status;

  /*
   * Status keeps an Error with no group for "ok", so that it needs no flag
   * beside it and stays 3 bytes. Private: only Status makes one, and it never
   * hands it out.
   */
  constexpr explicit Error(decltype(nullptr))
      : group_{ErrorGroup{nullptr}}, code_{0} {}

  // Whether this is the Error of an ok Status.
  constexpr bool IsNone() const { return group_ == ErrorGroup{nullptr}; }

  ErrorGroup group_;
  uint8_t code_;
};

}  // namespace utl

#endif  // UTL_SRC_UTL_ERROR_H_
