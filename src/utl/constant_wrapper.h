// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A value as a type: std::constant_wrapper, from C++26, for gnu++11, and
 *        UTL_CW() in place of std::cw.
 */

#ifndef UTL_SRC_UTL_CONSTANT_WRAPPER_H_
#define UTL_SRC_UTL_CONSTANT_WRAPPER_H_

#include "utl/internal/type_traits.h"

namespace utl {

/*!
 * \brief A value known when compiled, carried in a type: `Value`, of type `T`.
 *
 * It takes no memory and nothing to pass. A function_ref is made of one to
 * bind a member function to its object, the function being part of the code
 * rather than a pointer kept beside it:
 *
 * \code{.cpp}
 * utl::function_ref<void()> toggle{UTL_CW(&Led::Toggle), led};
 * \endcode
 *
 * Written through UTL_CW(), which gives `T` from the value.
 *
 * \tparam T The type of the value: an integer, an enum, or a pointer to a
 *           function, an object or a member.
 * \tparam Value The value.
 *
 * \stddiff The type comes before the value, as gnu++11 has no
 *          `template <auto>`; std::constant_wrapper takes the value alone. So
 *          there is no `std::cw<value>` either, but UTL_CW(value). It has
 *          `value`, `value_type` and `type`, and none of the standard's
 *          operators, as function_ref is all it is made for.
 */
template <typename T, T Value>
struct constant_wrapper {
  static constexpr T value{Value}; /*!< The value. */
  using value_type = T;            /*!< The type of the value. */
  using type = constant_wrapper;   /*!< This type. */
};

/*! \cond */
// The definition that a use of value as an object needs before C++17.
template <typename T, T Value>
constexpr T constant_wrapper<T, Value>::value;
/*! \endcond */

}  // namespace utl

// Braces, not parentheses, which would make
// `function_ref<void()> f(UTL_CW(Blink));` the declaration of a function;
// clang-format would break them apart.
// clang-format off
/*!
 * \brief A utl::constant_wrapper of the value given, as `std::cw<value>` is.
 *
 * The value is a constant: a function, by name or with `&`, a member
 * function, as `&Led::Toggle`, or an integer. A macro, as only a macro can
 * name the value's type and the value at once without `template <auto>`;
 * its arguments may hold commas, as `&Panel<8, 8>::Clear`.
 */
#define UTL_CW(...)                                                         \
  ::utl::constant_wrapper<                                                  \
      ::utl::internal::TemplateArgument<decltype(__VA_ARGS__)>, __VA_ARGS__>{}
// clang-format on

#endif  // UTL_SRC_UTL_CONSTANT_WRAPPER_H_
