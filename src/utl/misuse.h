// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The report of an access that is a mistake, with `UTL_CHECK_ACCESS` at
 *        1.
 */

#ifndef UTL_SRC_UTL_MISUSE_H_
#define UTL_SRC_UTL_MISUSE_H_

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "utl/config.h"

namespace utl {

/*!
 * \brief What was done that should not have been.
 *
 * Reported to ReportMisuse() when `UTL_CHECK_ACCESS` is 1.
 */
enum class Misuse : uint8_t {
  /*
   * An enum rather than a text at the place of the check: the checks lie in
   * templates, where F() would put one text into flash for every
   * instantiation, and where avr-gcc can end in a "section type conflict".
   */
  kResultValue,   /*!< `value()` of a Result with an error. */
  kOptionalValue, /*!< `*` or `->` of an empty optional. */
  /*!
   * \brief `[]` or `erase()` beyond the end, or an inplace_vector made of more
   *        elements than it holds.
   */
  kIndex,
  /*!
   * \brief `front()` or `back()` of an empty span, string_view or
   *        inplace_vector, or `pop_back()` of an empty one.
   */
  kEmpty,
  kRemove, /*!< `remove_prefix()` or `remove_suffix()` of more than there is. */
  /*!
   * \brief A function_ref made of a null function pointer, or of a member
   *        function and a null pointer to its object.
   */
  kNullFunction,
  kEmptyFunction, /*!< A call of an empty inplace_function. */
};

/*!
 * \brief Takes the report of a misuse, before the access happens.
 *
 * Called only with `UTL_CHECK_ACCESS` at 1. What follows the report is what the
 * access does unchecked. The default prints one line on `Serial`, such as
 * `utl: [] beyond the end`.
 *
 * The default is weak: a sketch whose `Serial` carries something else, or that
 * wants to halt or blink instead, defines its own in a `.cpp` file, and the
 * default and `Serial` are left out.
 *
 * \code{.cpp}
 * namespace utl {
 * void ReportMisuse(Misuse) {
 *   digitalWrite(LED_BUILTIN, HIGH);  // stays lit: something was misused
 * }
 * }  // namespace utl
 * \endcode
 *
 * \param[in] misuse What was done.
 */
void ReportMisuse(Misuse misuse);

/*!
 * \brief Prints a misuse as a line of the default report, without the end of
 *        the line.
 *
 * For a report of a sketch's own that prints as the default does.
 *
 * \param[out] out Where to print, such as `Serial`.
 * \param[in] misuse What was done.
 * \return The number of chars printed.
 */
size_t PrintMisuse(Print& out, Misuse misuse);

namespace internal {

/*
 * Reports `misuse` unless `ok`, and returns `ok`. constexpr, so that the
 * accessors that call it stay constexpr: the report lies in the branch of
 * ?: that a right access never takes, so a constant expression never reaches
 * it, and with the switch off there is none.
 */
#if UTL_CHECK_ACCESS
constexpr bool Check(bool ok, Misuse misuse) {
  return ok ? true : (ReportMisuse(misuse), false);
}
#else
constexpr bool Check(bool ok, Misuse /*misuse*/) { return ok; }
#endif

}  // namespace internal
}  // namespace utl

#endif  // UTL_SRC_UTL_MISUSE_H_
