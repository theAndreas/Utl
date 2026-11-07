// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A placement new of Utl's own, for inplace_function and inplace_vector;
 *        not part of the API.
 */

#ifndef UTL_SRC_UTL_INTERNAL_IN_PLACE_H_
#define UTL_SRC_UTL_INTERNAL_IN_PLACE_H_

#include <stddef.h>

namespace utl {
namespace internal {

// Where an object is made, for the operator new below.
struct InPlace {
  void* where;
};

}  // namespace internal
}  // namespace utl

/*
 * Placement new at an InPlace, in place of the one of <new>. Not noexcept, so
 * the compiler does not check its result for null, a branch that is never
 * taken; and not one the AVR core has to declare, which it does only since
 * 1.8.3. No delete to match: only a constructor that throws would call one,
 * which on the AVR, built without exceptions, none does, and on the host it
 * would be a branch in each placement that no test takes.
 */
// NOLINTNEXTLINE(misc-new-delete-overloads)
inline void* operator new(size_t /*size*/, utl::internal::InPlace place) {
  return place.where;
}

#endif  // UTL_SRC_UTL_INTERNAL_IN_PLACE_H_
