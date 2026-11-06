// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief std::invoke_r, for what is not a pointer to a member: how function_ref
 *        and inplace_function call what they hold; not part of the API.
 */

#ifndef UTL_SRC_UTL_INTERNAL_INVOKE_H_
#define UTL_SRC_UTL_INTERNAL_INVOKE_H_

namespace utl {
namespace internal {

// Calls `f` with `args` and returns its result as an R, converted
// implicitly, as a static_cast<R> would also take an explicit constructor.
template <typename R>
struct CallReturning {
  template <typename F, typename... Args>
  static R Call(F& f, Args&&... args) {
    return f(static_cast<Args&&>(args)...);
  }
};

// For R void, the result is dropped, whatever it is.
template <>
struct CallReturning<void> {
  template <typename F, typename... Args>
  static void Call(F& f, Args&&... args) {
    f(static_cast<Args&&>(args)...);
  }
};

}  // namespace internal
}  // namespace utl

#endif  // UTL_SRC_UTL_INTERNAL_INVOKE_H_
