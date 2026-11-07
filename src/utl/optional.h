// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A value or none: std::optional, from C++17, for gnu++11, and an
 *        optional of a reference, from C++26.
 */

#ifndef UTL_SRC_UTL_OPTIONAL_H_
#define UTL_SRC_UTL_OPTIONAL_H_

#include <stdint.h>

#include "utl/error.h"
#include "utl/internal/type_traits.h"
#include "utl/misuse.h"
#include "utl/optional_error.h"
#include "utl/result.h"

namespace utl {

/*! \cond */
// What a sketch does not see: the declaration ahead and the check of what
// AndThen() gets back.

template <typename T>
class optional;

namespace internal {

// kValue: whether T is an optional, what optional::AndThen() goes on with;
// see IsStatusOrResult in internal/fluent.h.
template <typename T>
struct IsOptionalImpl {
  static constexpr bool kValue{false};
};
template <typename T>
struct IsOptionalImpl<optional<T>> {
  static constexpr bool kValue{true};
};
template <typename T>
struct IsOptional {
  static constexpr bool kValue{IsOptionalImpl<RemoveCvRef<T>>::kValue};
};

}  // namespace internal

/*! \endcond */

/*!
 * \brief The type of nullopt, as std::nullopt_t.
 *
 * Not default-constructible, so that `optional = {}` stays an empty optional
 * rather than a nullopt_t.
 */
struct nullopt_t {
  /*! \brief Makes the one value of the type; nullopt is it. */
  constexpr explicit nullopt_t(uint8_t) {
    // The parameter only keeps the type from being made of {}; its type does
    // not matter.
  }
};

/*! \brief No value, as std::nullopt: what a function returns for none. */
constexpr nullopt_t nullopt{0};

/*!
 * \brief A value of T, or none.
 *
 * For "none" that is no error; a function that can fail returns a Result.
 *
 * \code{.cpp}
 * utl::optional<uint8_t> Find(char c) {
 *   ...
 *   return utl::nullopt;
 * }
 *
 * if (utl::optional<uint8_t> const index{Find(c)}) { Use(*index); }
 * Find(c).ValueOr(0);
 * Find(c).OkOr(FontError::kUnknownChar);  // a Result, with your error
 * \endcode
 *
 * It keeps the names of std::optional, except where Result does the same:
 * HasValue(), ValueOr(), OkOr(), AndThen(), Map() and OrElse() are named as
 * in Result, so that "a value or none" and "a value or an error" read alike.
 * The value and the flag lie side by side, as in a Result: an
 * `optional<uint8_t>` is 2 bytes.
 *
 * \tparam T The type of the value. It needs a default constructor, for the
 *           value of an empty optional. For a reference, use optional<T&>.
 *
 * \stddiff value() returns a Result with OptionalError::kEmpty where
 *          std::optional throws `bad_optional_access`, as a sketch is built
 *          without exceptions. reset() sets the value to `T{}` rather than
 *          destroying it. AndThen(), Map() and OrElse() are what C++23 calls
 *          `and_then()`, `transform()` and `or_else()`.
 */
template <typename T>
class optional {
 public:
  using value_type = T; /*!< The type of the value. */

  /*! \brief Makes an empty optional. */
  constexpr optional() : value_{}, has_value_{false} {}

  /*! \brief Makes an empty optional, from nullopt. */
  constexpr optional(nullopt_t)  // NOLINT(runtime/explicit)
      : value_(), has_value_(false) {}

  /*!
   * \brief Makes an optional of `value`.
   *
   * Implicit, as in the standard, so that a function returns its value as it
   * is.
   *
   * \param[in] value The value.
   */
  constexpr optional(T const& value)  // NOLINT(runtime/explicit)
      : value_(value), has_value_(true) {}

  /*! \brief Returns whether there is a value. */
  constexpr bool HasValue() const { return has_value_; }

  /*! \brief Returns whether there is a value, for `if (optional)`. */
  constexpr explicit operator bool() const { return has_value_; }

  /*!
   * \brief Returns the value, unchecked.
   *
   * \return The value, or `T()` if empty.
   * \pre HasValue(). With `UTL_CHECK_ACCESS` at 1 an empty one is reported
   *      first, as Misuse::kOptionalValue.
   */
  T& operator*() { return static_cast<void>(Checked()), value_; }

  /*! \copydoc operator*() */
  constexpr T const& operator*() const {
    return static_cast<void>(Checked()), value_;
  }

  /*!
   * \brief Returns the address of the value, unchecked, to reach its members.
   *
   * \pre HasValue(), as for operator*().
   */
  T* operator->() { return static_cast<void>(Checked()), &value_; }

  /*! \copydoc operator->() */
  constexpr T const* operator->() const {
    return static_cast<void>(Checked()), &value_;
  }

  /*!
   * \brief Returns the value, or OptionalError::kEmpty.
   *
   * A copy, as a Result holds a value.
   *
   * \stddiff Returns a Result where std::optional::value() throws.
   */
  Result<T> value() const {
    if (!has_value_) {
      return OptionalError::kEmpty;
    }
    return value_;
  }

  /*!
   * \brief Returns the value, or `fallback` if empty.
   *
   * \param[in] fallback What to return in place of a missing value.
   */
  constexpr T ValueOr(T const& fallback) const {
    return has_value_ ? value_ : fallback;
  }

  /*!
   * \brief Returns the value, or `error`.
   *
   * As Rust's `ok_or()`: an error of the caller's own group where value()
   * would give OptionalError::kEmpty.
   *
   * \param[in] error The error to return if empty.
   */
  Result<T> OkOr(Error const& error) const {
    if (!has_value_) {
      return error;
    }
    return value_;
  }

  /*!
   * \brief Returns `f(value)` if there is a value, or else an empty optional.
   *
   * \param[in] f A function that takes a `T const&` and returns an optional of
   *              any type; checked when compiled. For a plain value, use Map().
   * \return What `f` returns, or an empty optional of that type.
   */
  template <typename F>
  auto AndThen(F f) const -> decltype(f(internal::DeclVal<T const&>())) {
    static_assert(internal::IsOptional<decltype(f(
                      internal::DeclVal<T const&>()))>::kValue,
                  "AndThen: f returns an optional; for a plain value, use "
                  "Map()");
    if (!has_value_) {
      return nullopt;
    }
    return f(value_);
  }

  /*!
   * \brief Returns an optional of `f(value)` if there is a value, or else an
   *        empty one.
   *
   * \param[in] f A function that takes a `T const&`. If it returns a `U&`, the
   *              optional is an optional<U&>.
   */
  template <typename F, typename U = decltype(internal::DeclVal<F&>()(
                            internal::DeclVal<T const&>()))>
  optional<U> Map(F f) const {
    if (!has_value_) {
      return nullopt;
    }
    return f(value_);
  }

  /*!
   * \brief Returns this if there is a value, or else `f()`.
   *
   * \param[in] f A function that takes nothing and returns an optional<T>.
   */
  template <typename F>
  optional OrElse(F f) const {
    if (has_value_) {
      return *this;
    }
    return f();
  }

  /*!
   * \brief Returns the value, or else `f()`.
   *
   * As Rust's `unwrap_or_else()`: a fallback made only when it is needed.
   *
   * \param[in] f A function that takes nothing and returns a T.
   */
  template <typename F>
  T ValueOrElse(F f) const {
    if (has_value_) {
      return value_;
    }
    return f();
  }

  /*!
   * \brief Calls `f` with the value if there is one, and returns this as it is.
   *
   * To look at the value on its way, as Rust's `inspect()`.
   *
   * \param[in] f A function that takes a `T const&`, as it looks and does not
   *              change, and returns nothing; checked when compiled.
   */
  template <typename F>
  optional Inspect(F f) const {
    static_assert(
        internal::IsVoid<decltype(f(internal::DeclVal<T const&>()))>::kValue,
        "Inspect: f returns nothing; it only looks, and what it returned "
        "would be dropped unseen");
    if (has_value_) {
      f(value_);
    }
    return *this;
  }

  /*!
   * \brief Returns this if there is a value and `predicate` holds for it, or
   *        else an empty optional.
   *
   * As Rust's `filter()`.
   *
   * \param[in] predicate A function that takes a `T const&` and returns a
   *                      `bool`.
   */
  template <typename P>
  optional Filter(P predicate) const {
    if (has_value_ && predicate(value_)) {
      return *this;
    }
    return nullopt;
  }

  /*! \brief Empties it, and sets the value to `T{}`. */
  void reset() {
    value_ = T{};
    has_value_ = false;
  }

  /*!
   * \brief Makes the value from `args`, as `T(args...)`.
   *
   * \param[in] args What to make the value of.
   * \return The new value.
   */
  template <typename... Args>
  T& emplace(Args&&... args) {
    value_ = T(static_cast<Args&&>(args)...);
    has_value_ = true;
    return value_;
  }

  /*!
   * \brief Returns whether `a` and `b` are both empty or hold equal values.
   *
   * A value or nullopt converts on either side, as in `index == 2` and
   * `index == utl::nullopt`.
   *
   * \param[in] a An optional.
   * \param[in] b Another optional.
   */
  friend bool operator==(optional const& a, optional const& b) {
    return a.has_value_ == b.has_value_ &&
           (!a.has_value_ || a.value_ == b.value_);
  }

  /*!
   * \brief Returns whether `a` and `b` differ.
   *
   * \param[in] a An optional.
   * \param[in] b Another optional.
   */
  friend bool operator!=(optional const& a, optional const& b) {
    return !(a == b);
  }

 private:
  constexpr bool Checked() const {
    return internal::Check(has_value_, Misuse::kOptionalValue);
  }

  // Side by side, not in a union, as in Result: in return there is no
  // placement new and no union.
  T value_;
  bool has_value_;
};

/*!
 * \brief A reference to a T that lies elsewhere, or none.
 *
 * As std::optional<T&> from C++26: a pointer, 2 bytes on the Uno, null if
 * empty, with no flag beside it. T needs no default constructor.
 *
 * \code{.cpp}
 * utl::optional<Led&> FindLed(uint8_t pin);
 *
 * if (utl::optional<Led&> const led{FindLed(13)}) { led->On(); }
 * FindLed(13).ValueOr(spare).On();  // the LED, or the spare itself
 * \endcode
 *
 * Whether it may be empty is what sets it apart from a reference_wrapper; a
 * member that always refers to something is one of those.
 *
 * It binds an lvalue only, as a temporary would be gone before the first use.
 * Assigning one rebinds it, as in C++26, and never writes through it; to
 * write, use `*` or `->`. Its constness is its own: an `optional<T&> const`
 * still gives a `T&`. The fluent functions give their function the `T&`
 * itself, except Inspect() and Filter(), which only look and get a
 * `T const&`.
 *
 * \tparam T The type referred to, `const` for a reference that cannot write.
 *
 * \stddiff ValueOr() gives a `T&`, of the object referred to or of the
 *          fallback, which has to be an lvalue for that reason, where C++26's
 *          `value_or()` gives a copy: a copy would let
 *          `FindLed(13).ValueOr(spare).On()` switch on a copy of the LED.
 */
template <typename T>
class optional<T&> {
 public:
  using value_type = T; /*!< The type referred to. */

  /*! \brief Makes an empty optional. */
  constexpr optional() : pointer_{nullptr} {}

  /*! \brief Makes an empty optional, from nullopt. */
  constexpr optional(nullopt_t)  // NOLINT(runtime/explicit)
      : pointer_(nullptr) {}

  /*!
   * \brief Makes an optional that refers to `object`.
   *
   * Implicit, as in the standard, so that a function returns the object as it
   * is.
   *
   * \param[in] object The object, an lvalue, of a T that may overload unary
   *                   `&`.
   */
  constexpr optional(T& object)  // NOLINT(runtime/explicit)
      : pointer_(__builtin_addressof(object)) {
    // __builtin_addressof, what std::addressof is built on, takes the address
    // even of a T that overloads unary &.
  }

  /*!
   * \brief Deleted: a temporary would be gone before the first use.
   *
   * \note Not `explicit`, so that `optional<int const&> o = 5` does not compile
   *       either: an explicit one would be passed by there, and the 5 would
   *       bind through the `T const&` of the constructor above.
   */
  optional(T&&) = delete;  // NOLINT(runtime/explicit)

  /*!
   * \brief Makes an optional that refers to what `other` refers to, as an
   *        `optional<int&>` into an `optional<int const&>`, or an
   *        `optional<Derived&>` into an `optional<Base&>`.
   *
   * Only where a `U*` converts to a `T*`, so never from an `optional<long&>`
   * to an `optional<int&>`.
   *
   * \param[in] other The optional to take the reference of, or none.
   */
  template <typename U, typename = decltype(internal::ConvertTo<T*>(
                            internal::DeclVal<U*>()))>
  constexpr optional(optional<U&> const& other)  // NOLINT(runtime/explicit)
      : pointer_(other.HasValue() ? __builtin_addressof(*other) : nullptr) {}

  /*! \brief Returns whether it refers to an object. */
  constexpr bool HasValue() const { return pointer_ != nullptr; }

  /*! \brief Returns whether it refers to an object, for `if (optional)`. */
  constexpr explicit operator bool() const { return pointer_ != nullptr; }

  /*!
   * \brief Returns the object referred to, unchecked.
   *
   * \pre HasValue(). If empty, the access is undefined, as in the standard, as
   *      there is no T to give; with `UTL_CHECK_ACCESS` at 1 it is reported
   *      first, as Misuse::kOptionalValue.
   */
  constexpr T& operator*() const {
    // The analyzer sees the null reference that follows; it is what the
    // standard leaves undefined, not an oversight.
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return static_cast<void>(Checked()), *pointer_;
  }

  /*!
   * \brief Returns the address of the object referred to, to reach its members.
   *
   * \pre HasValue(), as for operator*().
   */
  constexpr T* operator->() const {
    return static_cast<void>(Checked()), pointer_;
  }

  /*! \brief Returns the reference, or OptionalError::kEmpty. */
  Result<T&> value() const {
    if (pointer_ == nullptr) {
      return OptionalError::kEmpty;
    }
    return *pointer_;
  }

  /*!
   * \brief Returns the object referred to, or `fallback` if empty.
   *
   * A reference either way.
   *
   * \param[in] fallback The object to return in place of a missing one, an
   *                     lvalue.
   */
  T& ValueOr(T& fallback) const {
    return pointer_ != nullptr ? *pointer_ : fallback;
  }

  /*! \brief Deleted: the `T&` returned would outlive a temporary fallback. */
  T& ValueOr(T&& fallback) const = delete;

  /*!
   * \brief Returns the reference, or `error`, as Rust's `ok_or()`.
   *
   * \param[in] error The error to return if empty.
   */
  Result<T&> OkOr(Error const& error) const {
    if (pointer_ == nullptr) {
      return error;
    }
    return *pointer_;
  }

  /*!
   * \brief Returns `f(object)` if it refers to one, or else an empty optional.
   *
   * \param[in] f A function that takes a `T&` and returns an optional of any
   *              type; checked when compiled. For a plain value, use Map().
   * \return What `f` returns, or an empty optional of that type.
   */
  template <typename F>
  auto AndThen(F f) const -> decltype(f(internal::DeclVal<T&>())) {
    static_assert(
        internal::IsOptional<decltype(f(internal::DeclVal<T&>()))>::kValue,
        "AndThen: f returns an optional; for a plain value, use "
        "Map()");
    if (pointer_ == nullptr) {
      return nullopt;
    }
    return f(*pointer_);
  }

  /*!
   * \brief Returns an optional of `f(object)` if it refers to one, or else an
   *        empty one.
   *
   * \param[in] f A function that takes a `T&`. If it returns a `U&`, the
   *              optional is an optional<U&>.
   */
  template <typename F, typename U = decltype(internal::DeclVal<F&>()(
                            internal::DeclVal<T&>()))>
  optional<U> Map(F f) const {
    if (pointer_ == nullptr) {
      return nullopt;
    }
    return f(*pointer_);
  }

  /*!
   * \brief Returns this if it refers to an object, or else `f()`.
   *
   * \param[in] f A function that takes nothing and returns an optional<T&>.
   */
  template <typename F>
  optional OrElse(F f) const {
    if (pointer_ != nullptr) {
      return *this;
    }
    return f();
  }

  /*!
   * \brief Returns the object referred to, or else the one `f()` refers to.
   *
   * \param[in] f A function that takes nothing and returns a `T&` to an object
   *              that outlives the call, as for ValueOr(); checked when
   *              compiled.
   */
  template <typename F>
  T& ValueOrElse(F f) const {
    static_assert(
        internal::IsLvalueReference<decltype(f())>::kValue,
        "f returns a T&: the reference returned would outlive a T made in f");
    if (pointer_ != nullptr) {
      return *pointer_;
    }
    return f();
  }

  /*!
   * \brief Calls `f` with the object if it refers to one, and returns this as
   *        it is.
   *
   * \param[in] f A function that takes a `T const&`, as it looks and does not
   *              change, and returns nothing; checked when compiled.
   */
  template <typename F>
  optional Inspect(F f) const {
    static_assert(
        internal::IsVoid<decltype(f(internal::DeclVal<T const&>()))>::kValue,
        "Inspect: f returns nothing; it only looks, and what it returned "
        "would be dropped unseen");
    if (pointer_ != nullptr) {
      f(static_cast<T const&>(*pointer_));
    }
    return *this;
  }

  /*!
   * \brief Returns this if it refers to an object and `predicate` holds for it,
   *        or else an empty optional.
   *
   * \param[in] predicate A function that takes a `T const&` and returns a
   *                      `bool`.
   */
  template <typename P>
  optional Filter(P predicate) const {
    if (pointer_ != nullptr && predicate(static_cast<T const&>(*pointer_))) {
      return *this;
    }
    return nullopt;
  }

  /*! \brief Empties it; the object referred to stays as it is. */
  void reset() { pointer_ = nullptr; }

  /*!
   * \brief Refers to `object` from now on, as in C++26.
   *
   * \param[in] object The object, an lvalue.
   * \return The object.
   */
  T& emplace(T& object) {
    pointer_ = __builtin_addressof(object);
    return object;
  }

  /*! \brief Deleted: a temporary would be gone before the first use. */
  T& emplace(T&& object) = delete;

  /*!
   * \brief Returns whether `a` and `b` are both empty or refer to equal
   *        objects.
   *
   * They compare by the objects, as in C++26, not by where these lie. nullopt
   * converts on either side, as in `led == utl::nullopt`.
   *
   * \param[in] a An optional.
   * \param[in] b Another optional.
   */
  friend bool operator==(optional const& a, optional const& b) {
    return a.pointer_ == nullptr
               ? b.pointer_ == nullptr
               : b.pointer_ != nullptr && *a.pointer_ == *b.pointer_;
  }

  /*!
   * \brief Returns whether `a` and `b` differ.
   *
   * \param[in] a An optional.
   * \param[in] b Another optional.
   */
  friend bool operator!=(optional const& a, optional const& b) {
    return !(a == b);
  }

  /*!
   * \brief Returns whether `a` refers to an object equal to `b`.
   *
   * \param[in] a An optional.
   * \param[in] b A value.
   */
  friend bool operator==(optional const& a, T const& b) {
    return a.pointer_ != nullptr && *a.pointer_ == b;
  }

  /*!
   * \brief Returns whether `b` refers to an object equal to `a`.
   *
   * \param[in] a A value.
   * \param[in] b An optional.
   */
  friend bool operator==(T const& a, optional const& b) { return b == a; }

  /*!
   * \brief Returns whether `a` is empty or refers to an object other than `b`.
   *
   * \param[in] a An optional.
   * \param[in] b A value.
   */
  friend bool operator!=(optional const& a, T const& b) { return !(a == b); }

  /*!
   * \brief Returns whether `b` is empty or refers to an object other than `a`.
   *
   * \param[in] a A value.
   * \param[in] b An optional.
   */
  friend bool operator!=(T const& a, optional const& b) { return !(b == a); }

 private:
  constexpr bool Checked() const {
    return internal::Check(pointer_ != nullptr, Misuse::kOptionalValue);
  }

  T* pointer_;
};

}  // namespace utl

#endif  // UTL_SRC_UTL_OPTIONAL_H_
