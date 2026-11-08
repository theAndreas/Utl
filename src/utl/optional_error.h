// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The errors of an optional: value() of one that holds none.
 */

#ifndef UTL_SRC_UTL_OPTIONAL_ERROR_H_
#define UTL_SRC_UTL_OPTIONAL_ERROR_H_

#include <avr/pgmspace.h>
#include <stdint.h>

#include "utl/error_group.h"

namespace utl {

/*! \brief What went wrong with an optional. */
enum class OptionalError : uint8_t {
  kEmpty, /*!< `value()` of an optional that holds none. */
};

/*!
 * \brief The name of the group, `"optional"`, in flash; its address is the
 *        group.
 *
 * A sketch that never calls `value()` of an optional does not reference it, and
 * the linker leaves it out.
 */
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
extern char const kOptionalErrorGroup[] PROGMEM;

/*! \brief Returns the group of a OptionalError, for argument-dependent lookup.
 */
constexpr ErrorGroup GetErrorGroup(OptionalError) {
  return ErrorGroup{kOptionalErrorGroup};
}

/*!
 * \brief Turns on the texts of the group.
 *
 * From then on an error of the group prints in words, `optional: ...`, rather
 * than as its code. See Messages.
 */
void UseOptionalMessages();

}  // namespace utl

#endif  // UTL_SRC_UTL_OPTIONAL_ERROR_H_
