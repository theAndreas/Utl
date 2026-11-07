// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The switches of the library.
 *
 * Each switch is set for the whole build, never in a sketch: the library's
 * own files and a group's `.cpp` have to see the same value. The Arduino IDE
 * passes no flags per sketch, so a switch goes into `platform.local.txt`
 * beside the `platform.txt` of the core:
 *
 * \code{.unparsed}
 * compiler.cpp.extra_flags=-DUTL_CHECK_ACCESS=1
 * \endcode
 *
 * With arduino-cli, it goes into a build property:
 *
 * \code{.unparsed}
 * --build-property "compiler.cpp.extra_flags=-DUTL_CHECK_ACCESS=1"
 * \endcode
 */

#ifndef UTL_SRC_UTL_CONFIG_H_
#define UTL_SRC_UTL_CONFIG_H_

/*!
 * \brief Reports an access that is a mistake before it happens, if 1.
 *
 * At 0, the default, nothing is checked, and nothing is compiled for it. At 1,
 * an access that the standard leaves undefined, or that gives a placeholder in
 * Utl, goes to utl::ReportMisuse() first:
 *
 * - `value()` of a Result with an error,
 * - `*` and `->` of an empty optional,
 * - `[]` beyond the end, `erase()` beyond the end of an `inplace_vector`, or
 *   one made of more elements than it holds,
 * - `front()` or `back()` of an empty span, string_view or `inplace_vector`,
 *   and `pop_back()` of an empty one,
 * - `remove_prefix()` or `remove_suffix()` of more than there is,
 * - a `function_ref` made of a null function pointer, or of a member
 *   function and a null pointer to its object,
 * - a call of an empty `inplace_function`, which gives `R()`.
 *
 * It is meant for finding a mistake while a sketch is written, as the DET of
 * AUTOSAR Classic is.
 *
 * \see Misuse, ReportMisuse()
 */
#ifndef UTL_CHECK_ACCESS
#define UTL_CHECK_ACCESS 0
#endif

#endif  // UTL_SRC_UTL_CONFIG_H_
