// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The errors of to_chars() and from_chars(), the group of charconv.h.
 */

#ifndef UTL_SRC_UTL_CHARCONV_ERROR_H_
#define UTL_SRC_UTL_CHARCONV_ERROR_H_

#include <avr/pgmspace.h>
#include <stdint.h>

#include "utl/error_group.h"

namespace utl {

/*!
 * \brief What went wrong converting a number: the three values of `std::errc`
 *        that std::to_chars() and std::from_chars() report, under their names.
 */
enum class CharconvError : uint8_t {
  /*! \brief No number where one should begin, or a base not 2 to 36. */
  kInvalidArgument,
  /*! \brief A number too large or too small for its type. */
  kOutOfRange,
  /*! \brief The text of the number does not fit into the buffer. */
  kValueTooLarge,
};

/*!
 * \brief The name of the group, `"charconv"`, in flash; its address is the
 *        group.
 *
 * A sketch that never converts does not reference it, and the linker leaves it
 * out.
 */
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
extern char const kCharconvErrorGroup[] PROGMEM;

/*! \brief Returns the group of a CharconvError, for argument-dependent lookup.
 */
constexpr ErrorGroup GetErrorGroup(CharconvError) {
  return ErrorGroup{kCharconvErrorGroup};
}

/*!
 * \brief Turns on the texts of the group.
 *
 * From then on an error of the group prints in words, `charconv: ...`, rather
 * than as its code. See Messages.
 */
void UseCharconvMessages();

}  // namespace utl

#endif  // UTL_SRC_UTL_CHARCONV_ERROR_H_
