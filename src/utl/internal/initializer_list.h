// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief std::initializer_list, which a braced list such as `{2, 3, 4}` needs
 *        and avr-gcc lacks; not part of the API.
 */

#ifndef UTL_SRC_UTL_INTERNAL_INITIALIZER_LIST_H_
#define UTL_SRC_UTL_INTERNAL_INITIALIZER_LIST_H_

#if __has_include(<initializer_list>)

#include <initializer_list>  // IWYU pragma: export

#else

#include <stddef.h>

/*
 * The compiler makes a braced list into a std::initializer_list itself, with
 * the private constructor below, and expects this class in namespace std,
 * laid out as GCC's own: a pointer to the elements, then their number. So it
 * is defined here only where no <initializer_list> is found, and under the
 * include guard of libstdc++'s, so that a library that brings that header, or
 * a copy of it, and this one do not define it twice.
 */
// The NOLINTs below: clang-tidy holds against these names that they belong
// to the standard library, which they do.
#ifndef _INITIALIZER_LIST  // NOLINT(bugprone-reserved-identifier)
// NOLINTNEXTLINE(readability-identifier-naming)
#define _INITIALIZER_LIST  // NOLINT(bugprone-reserved-identifier)

/*! \cond */
// NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std {

template <typename E>
// NOLINTNEXTLINE(readability-identifier-naming)
class initializer_list {  // NOLINT(bugprone-std-namespace-modification)
 public:
  using value_type = E;
  using reference = E const&;
  using const_reference = E const&;
  using size_type = size_t;
  using iterator = E const*;
  using const_iterator = E const*;

  constexpr initializer_list() noexcept : array_(nullptr), len_(0) {}

  constexpr size_type size() const noexcept { return len_; }
  constexpr const_iterator begin() const noexcept { return array_; }
  constexpr const_iterator end() const noexcept { return array_ + len_; }

 private:
  // The one the compiler calls for a braced list.
  constexpr initializer_list(const_iterator array, size_type len)
      : array_(array), len_(len) {}

  iterator array_;
  size_type len_;
};

template <typename E>
// NOLINTNEXTLINE(bugprone-std-namespace-modification)
constexpr E const* begin(initializer_list<E> list) noexcept {
  return list.begin();
}

template <typename E>
// NOLINTNEXTLINE(bugprone-std-namespace-modification)
constexpr E const* end(initializer_list<E> list) noexcept {
  return list.end();
}

}  // namespace std
/*! \endcond */

#endif  // _INITIALIZER_LIST

#endif  // __has_include(<initializer_list>)

#endif  // UTL_SRC_UTL_INTERNAL_INITIALIZER_LIST_H_
