// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A value, or the error that kept a function from returning one.
 */

#ifndef UTL_SRC_UTL_RESULT_H_
#define UTL_SRC_UTL_RESULT_H_

#include "utl/error.h"
#include "utl/internal/fluent.h"
#include "utl/internal/type_traits.h"
#include "utl/misuse.h"
#include "utl/status.h"

namespace utl {

/*!
 * \brief A value of T, or an Error.
 *
 * The value and a Status side by side: a `Result<uint8_t>` is 4 bytes on the
 * Uno.
 *
 * \code{.cpp}
 * utl::Result<uint8_t> GetRow(uint8_t index) {
 *   if (index >= kRows) {
 *     return PanelError::kOutOfRange;
 *   }
 *   return rows[index];
 * }
 *
 * bool const lit{GetRow(3)
 *                    .Map([](uint8_t row) { return row != 0; })
 *                    .ValueOr(false)};
 * \endcode
 *
 * The fluent functions follow Rust's `Result`: AndThen() goes on with the
 * value, Map() changes it, OrElse() recovers from the error. A function passed
 * to them returns by value; none of them calls it when it has nothing to give
 * it. A dropped Result warns, as `[[nodiscard]]`: a call that fetches a value
 * and drops it is a mistake almost every time.
 *
 * \tparam T The type of the value. It needs a default constructor, for the
 *           value of a Result with an error. For a reference, use Result<T&>;
 *           for no value, Result<void>.
 */
template <typename T>
class [[nodiscard]] Result : public internal::ValueFluent<Result<T>, T const&> {
 public:
  /*!
   * \brief Makes a Result of `value`.
   *
   * Implicit, like the conversions below, so that a function returns its value
   * or its error as it is: a Result is either, and nothing else.
   *
   * \param[in] value The value.
   */
  constexpr Result(T const& value)  // NOLINT(runtime/explicit)
      : value_{value} {}

  /*!
   * \brief Makes a Result of `error`, with `T()` as the value.
   *
   * \param[in] error The error.
   */
  constexpr Result(Error const& error)  // NOLINT(runtime/explicit)
      : value_{}, status_{error} {}

  /*!
   * \brief Makes a Result of a code of a group, as `FontError::kUnknownChar`.
   *
   * \param[in] code The code; see Error.
   */
  template <typename Code, typename = decltype(GetErrorGroup(Code{}))>
  constexpr Result(Code code)  // NOLINT(runtime/explicit)
      : value_{}, status_{code} {}

  /*! \brief Returns whether there is a value, no error. */
  bool IsOk() const { return status_.IsOk(); }

  /*! \brief Returns the error, or null if IsOk(), as Status::error(). */
  Error const* error() const { return status_.error(); }

  /*! \brief Returns ok or the error, to compare with a code or to print. */
  Status const& status() const { return status_; }

  /*!
   * \brief Returns the value.
   *
   * \return The value, or `T()` if there is an error.
   * \pre IsOk(); ask first, or use ValueOr(). With `UTL_CHECK_ACCESS` at 1, a
   *      call on an error is reported first, as Misuse::kResultValue.
   */
  T const& value() const {
    return static_cast<void>(internal::Check(IsOk(), Misuse::kResultValue)),
           value_;
  }

  /*!
   * \brief Returns the value, or `fallback` if there is an error.
   *
   * \param[in] fallback What to return in place of a missing value.
   */
  T ValueOr(T const& fallback) const { return IsOk() ? value_ : fallback; }

  /*!
   * \brief Returns the value, or else `f(*error())`.
   *
   * As Rust's `unwrap_or_else()`: a fallback made only when it is needed, and
   * from the error.
   *
   * \param[in] f A function that takes an `Error const&` and returns a T.
   */
  template <typename F>
  T ValueOrElse(F f) const {
    if (IsOk()) {
      return value_;
    }
    return f(*error());
  }

 private:
  friend class internal::ValueFluent<Result, T const&>;

  // The value, unchecked, for ValueFluent, which asks IsOk() first.
  T const& UncheckedValue() const { return value_; }

  /*
   * The value and the Status lie side by side, not in a union: in return the
   * class needs no tag, no placement new and no deviation under a safety
   * standard's rules on unions. What Result<T> and Result<T&> share lies in
   * internal/fluent.h; here is what differs: the storage, value(), ValueOr()
   * and ValueOrElse().
   */
  T value_;
  Status status_;
};

/*!
 * \brief A reference to a T that lies elsewhere, or an Error.
 *
 * A pointer and a Status, 5 bytes on the Uno, for a function that finds an
 * object or fails. T needs no default constructor.
 *
 * \code{.cpp}
 * utl::Result<Led&> GetLed(uint8_t pin) {
 *   if (pin >= kLeds) {
 *     return PanelError::kNoSuchLed;
 *   }
 *   return leds[pin];
 * }
 *
 * GetLed(13).Map([](Led& led) { led.On(); });
 * \endcode
 *
 * As Result<T>, with a `T&` in place of the value, which the fluent functions
 * give their function as it is. It binds an lvalue only, as a temporary would
 * be gone before the first use. Assigning one rebinds it, and never writes
 * through it.
 *
 * \tparam T The type referred to, `const` for a reference that cannot write.
 */
template <typename T>
class [[nodiscard]] Result<T&> : public internal::ValueFluent<Result<T&>, T&> {
 public:
  /*!
   * \brief Makes a Result that refers to `object`.
   *
   * \param[in] object The object, an lvalue, of a T that may overload unary
   *                   `&`.
   */
  constexpr Result(T& object)  // NOLINT(runtime/explicit)
      : pointer_{__builtin_addressof(object)} {
    // __builtin_addressof, what std::addressof is built on, takes the address
    // even of a T that overloads unary &.
  }

  /*!
   * \brief Deleted: a temporary would be gone before the first use.
   *
   * \note Not `explicit`, so that a `Result<int const&>` made from a 5 does not
   *       compile either: an explicit one would be passed by there, and the 5
   *       would bind through the `T const&` of the constructor above.
   */
  Result(T&&) = delete;  // NOLINT(runtime/explicit)

  /*!
   * \brief Makes a Result of `error`, which refers to nothing.
   *
   * \param[in] error The error.
   */
  constexpr Result(Error const& error)  // NOLINT(runtime/explicit)
      : pointer_{nullptr}, status_{error} {}

  /*!
   * \brief Makes a Result of a code of a group, as `FontError::kUnknownChar`.
   *
   * \param[in] code The code; see Error.
   */
  template <typename Code, typename = decltype(GetErrorGroup(Code{}))>
  constexpr Result(Code code)  // NOLINT(runtime/explicit)
      : pointer_{nullptr}, status_{code} {}

  /*!
   * \brief Makes a Result of what `other` holds, as a `Result<int&>` into a
   *        `Result<int const&>`, or a `Result<Derived&>` into a
   *        `Result<Base&>`.
   *
   * Only where a `U*` converts to a `T*`, so never from a `Result<long&>` to a
   * `Result<int&>`.
   *
   * \param[in] other The Result to take the reference or the error of.
   */
  template <typename U, typename = decltype(internal::ConvertTo<T*>(
                            internal::DeclVal<U*>()))>
  Result(Result<U&> const& other)  // NOLINT(runtime/explicit)
      : pointer_{other.IsOk() ? __builtin_addressof(other.value()) : nullptr},
        status_{other.status()} {}

  /*! \brief Returns whether there is a reference, no error. */
  bool IsOk() const { return status_.IsOk(); }

  /*! \brief Returns the error, or null if IsOk(), as Status::error(). */
  Error const* error() const { return status_.error(); }

  /*! \brief Returns ok or the error, to compare with a code or to print. */
  Status const& status() const { return status_; }

  /*!
   * \brief Returns the object referred to.
   *
   * \pre IsOk(); ask first, or use ValueOr(). On an error the access is
   *      undefined, as there is no T to give; with `UTL_CHECK_ACCESS` at 1 it
   *      is reported first, as Misuse::kResultValue.
   *
   * \stddiff Result<T> gives `T()` on an error; Result<T&> has nothing to give.
   */
  T& value() const {
    // The analyzer sees the null reference that follows an error; it is what
    // the standard leaves undefined, not an oversight.
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return static_cast<void>(internal::Check(IsOk(), Misuse::kResultValue)),
           *pointer_;
  }

  /*!
   * \brief Returns the object referred to, or `fallback` if there is an error.
   *
   * A reference either way.
   *
   * \param[in] fallback The object to return in place of a missing one, an
   *                     lvalue.
   */
  T& ValueOr(T& fallback) const { return IsOk() ? *pointer_ : fallback; }

  /*! \brief Deleted: the `T&` returned would outlive a temporary fallback. */
  T& ValueOr(T&& fallback) const = delete;

  /*!
   * \brief Returns the object referred to, or else the one `f(*error())` refers
   *        to.
   *
   * \param[in] f A function that takes an `Error const&` and returns a `T&` to
   *              an object that outlives the call; checked when compiled, as a
   *              T made in `f` would be gone before the caller got to it.
   */
  template <typename F>
  T& ValueOrElse(F f) const {
    static_assert(
        internal::IsLvalueReference<decltype(f(*error()))>::kValue,
        "f returns a T&: the reference returned would outlive a T made in f");
    if (IsOk()) {
      return *pointer_;
    }
    return f(*error());
  }

 private:
  friend class internal::ValueFluent<Result, T&>;

  // The object referred to, unchecked, for ValueFluent, which asks IsOk()
  // first.
  T& UncheckedValue() const { return *pointer_; }

  T* pointer_;
  Status status_;
};

/*!
 * \brief The Result of a function that returns no value: a Status.
 *
 * Ok or an Error, as Rust's `Result<(), E>`. It is a Status, so that it has all
 * of it, and converts to one and back. It is made as a Status is: ok, from an
 * Error, or from a code. It exists for code that is generic in T, above all
 * Map() with a function that returns nothing; a function of a sketch returns a
 * Status.
 *
 * Not `[[nodiscard]]`, as a Status is not: it has no value to lose.
 */
template <>
class Result<void> : public Status {
 public:
  /*! \cond */
  /*
   * The constructors of Status, from an Error and from a code, which the
   * class block names: Doxygen takes the using-declaration for a function
   * whose parameters it cannot see.
   */
  using Status::Status;
  /*! \endcond */

  /*! \brief Makes an ok Result. */
  constexpr Result() = default;

  /*! \brief Returns an ok Result. */
  // It hides Status::Ok() on purpose: code generic in T calls Result<T>::Ok()
  // and gets a Result, not a Status.
  // cppcheck-suppress duplInheritedMember
  static constexpr Result Ok() { return Result{}; }

  /*!
   * \brief Makes a Result of `status`, which is the same thing.
   *
   * Implicit, as the conversions of Status are.
   *
   * \param[in] status Ok, or an error.
   */
  constexpr Result(Status const& status)  // NOLINT(runtime/explicit)
      : Status{status} {}

  /*! \brief Returns this, as the Status it is. */
  Status const& status() const { return *this; }

  /*!
   * \brief Returns a Result of `f()` if there is no error, or else the error.
   *
   * Makes a value from "ok", as Rust's `map()` on a `Result<(), E>`.
   *
   * \param[in] f A function that takes nothing.
   * \return A Result of what `f` returns, or the error.
   */
  template <typename F, typename U = decltype(internal::DeclVal<F&>()())>
  Result<U> Map(F f) const {
    if (!IsOk()) {
      return *error();
    }
    return internal::MapCall<U>::Call(f);
  }
};

namespace internal {

/*! \cond */
// Declared in internal/fluent.h, which says what it does.

template <typename U>
struct MapCall {
  template <typename F, typename... Args>
  static Result<U> Call(F& f, Args&... args) {
    return f(args...);
  }
};

template <>
struct MapCall<void> {
  template <typename F, typename... Args>
  static Result<void> Call(F& f, Args&... args) {
    f(args...);
    return Result<void>::Ok();
  }
};

/*! \endcond */

}  // namespace internal

}  // namespace utl

#endif  // UTL_SRC_UTL_RESULT_H_
