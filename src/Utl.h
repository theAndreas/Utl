// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The header a sketch includes, before any other that uses Utl.
 *
 * It includes all of Utl. arduino-cli finds a library only by a header that
 * lies directly in its `src/`, not by `<utl/error.h>`, and it goes through a
 * sketch's includes in order. Once this header has found the library, `src/`
 * is on the include path, and `<utl/...>` works in every file after it.
 *
 * \code{.cpp}
 * #include <Utl.h>
 * #include "font_error.h"  // which includes <utl/error_group.h>
 * \endcode
 */

/*
 * The name is the library's, as Arduino expects, not lower case as Google's.
 *
 * It includes all of utl, against Google's "include what you use", which this
 * library follows everywhere else. Weighed on 2026-11-02:
 *
 * - The header in src/ cannot be left out, so the question was only whether
 *   it opens the door and nothing more, with a sketch including <utl/...> for
 *   each part after it, or brings everything. One line is what a sketch
 *   expects of an Arduino library.
 * - It costs nothing on the target: Basics, built with this header as it is
 *   and with one that includes only config.h, the parts Basics uses included
 *   by name, takes 2376 bytes of flash and 208 of RAM either way. A template
 *   or an inline function that is not used makes no code, and the linker
 *   leaves out the rest.
 * - What it costs: every file that includes it compiles all of utl, and all
 *   of it is in scope, nullopt and the groups of the library among it.
 *
 * A file may still include <utl/...> by name, after this header. The export
 * pragmas tell clang-tidy's include-cleaner that this header provides what
 * it includes.
 */

#ifndef UTL_SRC_UTL_H_
#define UTL_SRC_UTL_H_

#include "utl/array.h"              // IWYU pragma: export
#include "utl/charconv.h"           // IWYU pragma: export
#include "utl/charconv_error.h"     // IWYU pragma: export
#include "utl/config.h"             // IWYU pragma: export
#include "utl/constant_wrapper.h"   // IWYU pragma: export
#include "utl/error.h"              // IWYU pragma: export
#include "utl/error_group.h"        // IWYU pragma: export
#include "utl/flash_string_view.h"  // IWYU pragma: export
#include "utl/function_ref.h"       // IWYU pragma: export
#include "utl/index_error.h"        // IWYU pragma: export
#include "utl/inplace_function.h"   // IWYU pragma: export
#include "utl/inplace_vector.h"     // IWYU pragma: export
#include "utl/messages.h"           // IWYU pragma: export
#include "utl/misuse.h"             // IWYU pragma: export
#include "utl/optional.h"           // IWYU pragma: export
#include "utl/optional_error.h"     // IWYU pragma: export
#include "utl/reference_wrapper.h"  // IWYU pragma: export
#include "utl/result.h"             // IWYU pragma: export
#include "utl/span.h"               // IWYU pragma: export
#include "utl/status.h"             // IWYU pragma: export
#include "utl/string_view.h"        // IWYU pragma: export

/*!
 * \namespace utl
 * \brief Everything Utl provides.
 */

/*!
 * \namespace utl::internal
 * \brief What Utl builds itself from; not part of its API.
 *
 * A sketch does not name anything in it. It may change in any release.
 */

#endif  // UTL_SRC_UTL_H_
