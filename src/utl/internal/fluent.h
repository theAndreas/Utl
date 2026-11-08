// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The fluent functions that Status, Result<T> and Result<T&> share.
 *
 * What they do with an error lies in ErrorFluent, what they do with a value in
 * ValueFluent. Each class derives from what it has and keeps what differs
 * itself. The bases hold nothing: a Status stays 3 bytes, and a Result its
 * value and a Status.
 */

#ifndef UTL_SRC_UTL_INTERNAL_FLUENT_H_
#define UTL_SRC_UTL_INTERNAL_FLUENT_H_

#include "utl/error.h"
#include "utl/internal/type_traits.h"

namespace utl {

/*! \cond */
// What a sketch does not see: the declarations ahead and the helpers of the
// functions below.

class Status;
template <typename T>
class Result;

namespace internal {

/*
 * Calls `f(args...)` and makes a Result<U> of what it returns, for Map(). The
 * specialization for void calls `f` and returns ok, as `return f(args...);`
 * cannot make a Result of nothing. The arguments are passed on as they are, so
 * that the T& of a Result<T&> reaches `f` as a T&, not a T const&. Defined in
 * result.h, below Result<void>.
 */
template <typename U>
struct MapCall;

/*
 * What the functions passed to the fluent functions may return, checked with
 * a static_assert and a message that says what to use instead: the error
 * that a template deep in AndThen() or MapError() gives otherwise names
 * neither. Each takes the type as the function returns it, by value, by
 * reference or const.
 */

// kValue: whether T is a Status or a Result, what AndThen() goes on with.
template <typename T>
struct IsStatusOrResultImpl {
  static constexpr bool kValue{false};
};
template <>
struct IsStatusOrResultImpl<Status> {
  static constexpr bool kValue{true};
};
template <typename T>
struct IsStatusOrResultImpl<Result<T>> {
  static constexpr bool kValue{true};
};
template <typename T>
struct IsStatusOrResult {
  static constexpr bool kValue{IsStatusOrResultImpl<RemoveCvRef<T>>::kValue};
};

// kValue: whether T is an Error or a code of a group, what MapError() turns
// an error into; never a Status, which could be ok.
template <typename T, typename = void>
struct IsErrorOrCodeImpl {
  static constexpr bool kValue{false};
};
template <typename T>
struct IsErrorOrCodeImpl<T, Void<decltype(GetErrorGroup(T{}))>> {
  static constexpr bool kValue{true};
};
template <>
struct IsErrorOrCodeImpl<Error, void> {
  static constexpr bool kValue{true};
};
template <typename T>
struct IsErrorOrCode {
  static constexpr bool kValue{IsErrorOrCodeImpl<RemoveCvRef<T>>::kValue};
};

/*! \endcond */

/*!
 * \brief What Status, Result<T> and Result<T&> do with an error.
 *
 * \tparam Self The class that derives from it, which each function returns.
 */
template <typename Self>
class ErrorFluent {
 public:
  /*!
   * \brief Returns this if there is no error, or else `f(*error())`.
   *
   * Recovers from an error: `f` returns a value in place of the missing one,
   * another error, or what recovers, such as Status::Ok().
   *
   * \param[in] f A function that takes an `Error const&`.
   * \return This, or what `f` returns.
   */
  template <typename F>
  Self OrElse(F f) const {
    if (AsSelf().IsOk()) {
      return AsSelf();
    }
    return f(*AsSelf().error());
  }

  /*!
   * \brief Calls `f` with the error if there is one, and returns this as it is.
   *
   * To look at an error on its way, to print it, say, as Rust's
   * `inspect_err()`.
   *
   * \param[in] f A function that takes an `Error const&` and returns nothing;
   *              checked when compiled, as what it returned would be dropped
   *              unseen.
   * \return This, unchanged.
   */
  template <typename F>
  Self InspectError(F f) const {
    static_assert(IsVoid<decltype(f(DeclVal<Error const&>()))>::kValue,
                  "InspectError: f returns nothing; it only looks, and what "
                  "it returned would be dropped unseen");
    if (!AsSelf().IsOk()) {
      f(*AsSelf().error());
    }
    return AsSelf();
  }

  /*!
   * \brief Returns this if there is no error, or else the error `f` makes of
   *        it.
   *
   * As Rust's `map_err()`: to say an error of a lower layer in a group of
   * one's own.
   *
   * \param[in] f A function that takes an `Error const&` and returns an Error
   *              or a code, never ok; checked when compiled. To recover, use
   *              OrElse().
   * \return This, or the error `f` returns.
   */
  template <typename F>
  Self MapError(F f) const {
    static_assert(IsErrorOrCode<decltype(f(DeclVal<Error const&>()))>::kValue,
                  "MapError: f returns an Error or a code; to recover from the "
                  "error, use OrElse()");
    if (AsSelf().IsOk()) {
      return AsSelf();
    }
    Error const mapped{f(*AsSelf().error())};
    return mapped;
  }

 private:
  // Only Self makes one, and ValueFluent on its way to Self, so that no class
  // derives from ErrorFluent<Other> by mistake.
  friend Self;
  template <typename, typename>
  friend class ValueFluent;
  constexpr ErrorFluent() = default;

  // Self as what it is. It is asked only for IsOk() and error(), which each
  // Self has.
  Self const& AsSelf() const { return static_cast<Self const&>(*this); }
};

/*!
 * \brief What Result<T> and Result<T&> do with a value, and through ErrorFluent
 *        with an error.
 *
 * A function passed to these returns by value; none of them calls it when it
 * has nothing to give.
 *
 * \tparam Self The class that derives from it.
 * \tparam Value What value() gives and what `f` gets, a `T const&` or a `T&`.
 */
template <typename Self, typename Value>
class ValueFluent : public ErrorFluent<Self> {
 public:
  /*!
   * \brief Returns `f(value())` if there is no error, or else the error.
   *
   * \param[in] f A function that takes the value and returns a Result of any
   *              type or a Status; checked when compiled. For a plain value,
   *              use Map().
   * \return What `f` returns, or the error.
   */
  template <typename F>
  auto AndThen(F f) const -> decltype(f(DeclVal<Value>())) {
    static_assert(IsStatusOrResult<decltype(f(DeclVal<Value>()))>::kValue,
                  "AndThen: f returns a Status or a Result; for a plain value, "
                  "use Map()");
    if (!AsSelf().IsOk()) {
      return *AsSelf().error();
    }
    return f(AsSelf().UncheckedValue());
  }

  /*!
   * \brief Returns a Result of `f(value())` if there is no error, or else the
   *        error.
   *
   * \param[in] f A function that takes the value. If it returns nothing, it is
   *              called for what it does, and the Result is a Result<void>; if
   *              it returns a `U&`, a Result<U&>.
   * \return A Result of what `f` returns, or the error.
   */
  template <typename F, typename U = decltype(DeclVal<F&>()(DeclVal<Value>()))>
  Result<U> Map(F f) const {
    if (!AsSelf().IsOk()) {
      return *AsSelf().error();
    }
    return MapCall<U>::Call(f, AsSelf().UncheckedValue());
  }

  /*!
   * \brief Calls `f` with the value if there is no error, and returns this as
   *        it is.
   *
   * To look at the value on its way, as Rust's `inspect()`.
   *
   * \param[in] f A function that takes a `T const&`, as it looks and does not
   *              change, and returns nothing; checked when compiled.
   * \return This, unchanged.
   */
  template <typename F>
  Self Inspect(F f) const {
    static_assert(IsVoid<decltype(f(DeclVal<Looked>()))>::kValue,
                  "Inspect: f returns nothing; it only looks, and what it "
                  "returned would be dropped unseen");
    if (AsSelf().IsOk()) {
      f(static_cast<Looked>(AsSelf().UncheckedValue()));
    }
    return AsSelf();
  }

 private:
  // Only Self makes one, as for ErrorFluent. It derives from ErrorFluent,
  // rather than Self from both, so that a Result has one base, not two.
  friend Self;
  constexpr ValueFluent() = default;

  using Looked = RemoveReference<Value> const&;

  /*
   * Self as what it is. The value is taken through UncheckedValue(), which
   * Self keeps private and opens to this as a friend, and only when there is
   * one: value() would check again what IsOk() has just said, which with
   * UTL_CHECK_ACCESS at 1 cost SelfCheck 360 bytes.
   */
  Self const& AsSelf() const { return static_cast<Self const&>(*this); }
};

}  // namespace internal
}  // namespace utl

#endif  // UTL_SRC_UTL_INTERNAL_FLUENT_H_
