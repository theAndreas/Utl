// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The errors of an index, the group of array, span, inplace_vector and
 *        the views of text.
 */

#ifndef UTL_SRC_UTL_INDEX_ERROR_H_
#define UTL_SRC_UTL_INDEX_ERROR_H_

#include <avr/pgmspace.h>
#include <stdint.h>

#include "utl/error_group.h"

namespace utl {

/*! \brief What went wrong with an index. */
enum class IndexError : uint8_t {
  kOutOfRange, /*!< The index, or the part asked for, lies beyond the end. */
  kFull,       /*!< An inplace_vector has no room for another element. */
};

/*!
 * \brief The name of the group, `"index"`, in flash; its address is the group.
 *
 * A sketch that never calls `at()`, `first()`, `last()`, `subspan()`,
 * `substr()`, or what adds to an inplace_vector, does not reference it, and the
 * linker leaves it out.
 */
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
extern char const kIndexErrorGroup[] PROGMEM;

/*! \brief Returns the group of a IndexError, for argument-dependent lookup. */
constexpr ErrorGroup GetErrorGroup(IndexError) {
  return ErrorGroup(kIndexErrorGroup);
}

/*!
 * \brief Turns on the texts of the group.
 *
 * From then on an error of the group prints in words, `index: ...`, rather than
 * as its code. See Messages.
 */
void UseIndexMessages();

}  // namespace utl

#endif  // UTL_SRC_UTL_INDEX_ERROR_H_
