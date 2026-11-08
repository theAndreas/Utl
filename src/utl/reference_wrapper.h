// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A reference that copies: std::reference_wrapper, ref() and cref(), for
 *        gnu++11.
 */

#ifndef UTL_SRC_UTL_REFERENCE_WRAPPER_H_
#define UTL_SRC_UTL_REFERENCE_WRAPPER_H_

#include "utl/internal/type_traits.h"

namespace utl {

/*!
 * \brief A reference to a T that can be copied and assigned.
 *
 * Inside a pointer, 2 bytes on the Uno, but one that is never null and owns
 * nothing. It is meant for a member that refers to an object another owns:
 * a `T&` member would make the class neither copyable nor assignable, and a
 * `T*` would leave open whether it may be null and who deletes it.
 *
 * \code{.cpp}
 * class Panel {
 *  public:
 *   explicit Panel(Display& display) : display_(display) {}
 *   void Clear() { display_.get().Clear(); }
 *
 *  private:
 *   utl::reference_wrapper<Display> display_;
 * };
 * \endcode
 *
 * It binds an lvalue only, never a temporary, which would be gone before the
 * first use. There is no default constructor, so a class with one as a member
 * has none either unless it binds the member; a reference that may be absent
 * is an `optional<T&>` instead.
 *
 * Assigning one rebinds it, as in the standard: `a = b` makes `a` refer to
 * what `b` refers to and leaves both objects as they are. To write through it,
 * use get().
 *
 * \tparam T The type referred to, `const` for a reference that cannot write.
 *
 * \stddiff No `operator()`, which calls a function referred to as `std::invoke`
 *          does: it is the one costly part, and a member that refers to an
 *          object needs none of it.
 */
template <typename T>
class reference_wrapper {
 public:
  using type = T; /*!< The type referred to. */

  /*!
   * \brief Makes a reference to `object`.
   *
   * Implicit, as in the standard, so that a utl::array of them is written as
   * the objects they refer to, `{{red, green}}`. An object of a class derived
   * from T binds as well, and one of a T that overloads unary `&`.
   *
   * \param[in] object The object, an lvalue.
   */
  constexpr reference_wrapper(T& object)  // NOLINT(runtime/explicit)
      : pointer_(__builtin_addressof(object)) {
    // __builtin_addressof, what std::addressof is built on, takes the address
    // even of a T that overloads unary &.
  }

  /*!
   * \brief Deleted: a temporary would be gone before the first use.
   *
   * \note Deleted rather than left out, and not `explicit`, so that
   *       `reference_wrapper<int const> r = 5` does not compile either: without
   *       it, or with an explicit one passed by there, the 5 would bind through
   *       the `T const&` of the constructor above.
   */
  reference_wrapper(T&&) = delete;  // NOLINT(runtime/explicit)

  /*!
   * \brief Makes a reference to the object `other` refers to, as one of `int`
   *        into one of `int const`, or one of `Derived` into one of `Base`.
   *
   * Implicit, as in the standard, and only where a `U*` converts to a `T*`, so
   * never from one of `long` to one of `int`.
   *
   * \param[in] other The reference to take the object of.
   */
  template <typename U, typename = decltype(internal::ConvertTo<T*>(
                            internal::DeclVal<U*>()))>
  constexpr reference_wrapper(  // NOLINT(runtime/explicit)
      reference_wrapper<U> const& other)
      : pointer_{__builtin_addressof(other.get())} {}

  /*! \brief Returns the object referred to. */
  constexpr T& get() const { return *pointer_; }

  /*!
   * \brief Returns the object referred to, so that it passes as a `T&`.
   *
   * Implicit, as in the standard, for a function that takes a `T&`.
   */
  constexpr operator T&() const { return *pointer_; }

 private:
  T* pointer_;
};

/*!
 * \brief Returns a reference_wrapper of `object`, its type taken from the
 *        argument.
 *
 * What C++17 deduces from `reference_wrapper r = object`, which gnu++11
 * cannot.
 *
 * \param[in] object The object, an lvalue.
 */
template <typename T>
constexpr reference_wrapper<T> ref(T& object) {
  return reference_wrapper<T>{object};
}

/*!
 * \brief Returns `wrapper` itself, not a reference to it.
 *
 * \param[in] wrapper A reference already.
 */
template <typename T>
constexpr reference_wrapper<T> ref(reference_wrapper<T> wrapper) {
  return wrapper;
}

/*! \brief Deleted: a temporary would be gone before the first use. */
template <typename T>
void ref(T const&&) = delete;

/*!
 * \brief Returns a reference_wrapper of `object` that cannot write.
 *
 * \param[in] object The object, an lvalue.
 */
template <typename T>
constexpr reference_wrapper<T const> cref(T const& object) {
  return reference_wrapper<T const>{object};
}

/*!
 * \brief Returns a reference that cannot write to what `wrapper` refers to.
 *
 * \param[in] wrapper A reference already.
 */
template <typename T>
constexpr reference_wrapper<T const> cref(reference_wrapper<T> wrapper) {
  return reference_wrapper<T const>{wrapper.get()};
}

/*! \brief Deleted: a temporary would be gone before the first use. */
template <typename T>
void cref(T const&&) = delete;

}  // namespace utl

#endif  // UTL_SRC_UTL_REFERENCE_WRAPPER_H_
