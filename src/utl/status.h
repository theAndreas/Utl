// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief What a function returns that has nothing else to return: ok, or an
 *        error.
 */

#ifndef UTL_SRC_UTL_STATUS_H_
#define UTL_SRC_UTL_STATUS_H_

#include <Arduino.h>
#include <stddef.h>

#include "utl/error.h"
#include "utl/internal/fluent.h"
#include "utl/internal/type_traits.h"

namespace utl {

/*!
 * \brief Ok, or an Error.
 *
 * 3 bytes on the Uno, as Rust's `Result<(), E>` or `absl::Status`.
 *
 * \code{.cpp}
 * utl::Status SetChar(char c) {
 *   if (c < ' ') {
 *     return FontError::kUnknownChar;
 *   }
 *   return utl::Status::Ok();
 * }
 *
 * utl::Status const status{SetChar(c)};
 * if (status == FontError::kUnknownChar) { ... }
 * if (utl::Error const* error{status.error()}) {
 *   error->group().Name();
 * }
 * \endcode
 *
 * The fluent functions follow Rust: AndThen() goes on while there is no error,
 * ConsumeError() ends a chain. OrElse(), InspectError() and MapError(), which
 * Result has as well, come from its base.
 *
 * \note Not `[[nodiscard]]`, unlike Result and Error: a sketch drops the Status
 *       of a call that draws or sets, `SetText("Hi!")`, as a matter of course.
 *       A library marks a function whose Status must be looked at itself:
 *       `[[nodiscard]] utl::Status Begin();`.
 */
class Status : public internal::ErrorFluent<Status> {
 public:
  /*! \brief Makes an ok Status; Ok() says the same where it is returned. */
  constexpr Status() : error_{nullptr} {}

  /*! \brief Returns an ok Status. */
  static constexpr Status Ok() { return Status{}; }

  /*!
   * \brief Makes a Status of `error`.
   *
   * Implicit, so that a function that returns a Status returns its error as it
   * is.
   *
   * \param[in] error The error.
   */
  constexpr Status(Error const& error)  // NOLINT(runtime/explicit)
      : error_{error} {}

  /*!
   * \brief Makes a Status of a code of a group, as `FontError::kUnknownChar`.
   *
   * \param[in] code The code; see Error.
   */
  template <typename Code, typename = decltype(GetErrorGroup(Code{}))>
  constexpr Status(Code code)  // NOLINT(runtime/explicit)
      : error_{code} {
    // Without it, `return FontError::kUnknownChar;` would need two conversions
    // in a row, which C++ does not make implicitly.
  }

  /*! \brief Returns whether there is no error. */
  bool IsOk() const { return error_.IsNone(); }

  /*!
   * \brief Returns the error, or null if IsOk().
   *
   * A pointer, so that no one gets hold of an Error that is none.
   */
  Error const* error() const { return IsOk() ? nullptr : &error_; }

  /*! \brief Returns the text of the error, or `ok`, from flash. */
  __FlashStringHelper const* Message() const;

  /*!
   * \brief Prints `ok`, or the error as Error::PrintTo() does.
   *
   * \param[out] out Where to print, such as `Serial`.
   * \return The number of chars printed.
   */
  size_t PrintTo(Print& out) const;

  /*!
   * \brief Returns `f()` if there is no error, or else the error.
   *
   * `f` is not called if there is an error.
   *
   * \param[in] f A function that takes nothing and returns a Status or a
   *              Result; checked when compiled, with Map() of a Result<void>
   *              suggested for a plain value.
   * \return What `f` returns, or the error.
   */
  template <typename F>
  auto AndThen(F f) const -> decltype(f()) {
    static_assert(internal::IsStatusOrResult<decltype(f())>::kValue,
                  "AndThen: f returns a Status or a Result; for a plain value, "
                  "use Map()");
    if (!IsOk()) {
      return error_;
    }
    return f();
  }

  /*!
   * \brief Calls `f` with the error if there is one, and returns nothing.
   *
   * The end of a chain, where the error is dealt with:
   *
   * \code{.cpp}
   * Receive()
   *     .Map([](char c) { Serial.println(c); })
   *     .ConsumeError([](utl::Error const& e) { e.PrintTo(Serial); });
   * \endcode
   *
   * Only on a Status, and so on a Result<void>: on a Result<T> it would drop
   * the value unseen, which `[[nodiscard]]` there keeps from happening.
   *
   * \param[in] f A function that takes an `Error const&` and returns nothing;
   *              checked when compiled, as a Status it returned would be
   *              dropped unseen.
   */
  template <typename F>
  void ConsumeError(F f) const {
    static_assert(internal::IsVoid<decltype(f(error_))>::kValue,
                  "ConsumeError: f returns nothing; a Status or a Result it "
                  "returned would be dropped unseen");
    if (!IsOk()) {
      f(error_);
    }
  }

  /*!
   * \brief Returns whether `a` and `b` are both ok or hold the same error.
   *
   * An Error or a code converts on either side, as in
   * `status == FontError::kNullText`.
   *
   * \param[in] a A Status.
   * \param[in] b Another Status.
   */
  friend bool operator==(Status const& a, Status const& b) {
    // Two ok ones are equal, as both hold the same null group and code 0.
    return a.error_ == b.error_;
  }

  /*!
   * \brief Returns whether `a` and `b` differ.
   *
   * \param[in] a A Status.
   * \param[in] b Another Status.
   */
  friend bool operator!=(Status const& a, Status const& b) { return !(a == b); }

 private:
  // With a null group if ok.
  Error error_;
};

}  // namespace utl

#endif  // UTL_SRC_UTL_STATUS_H_
