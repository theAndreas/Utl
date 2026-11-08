// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Integers to text and back: std::to_chars() and std::from_chars(), from
 *        C++17, for gnu++11, with a Result in place of `std::errc`.
 *
 * A number is converted in the width of its type: to_chars() of a `uint8_t`
 * links an 8-bit division only, where `Serial.print()` of one links the 32-bit
 * division of an `unsigned long`.
 */

#ifndef UTL_SRC_UTL_CHARCONV_H_
#define UTL_SRC_UTL_CHARCONV_H_

#include <stddef.h>
#include <stdint.h>

#include "utl/charconv_error.h"
#include "utl/internal/charconv.h"
#include "utl/result.h"
#include "utl/span.h"
#include "utl/string_view.h"

namespace utl {

/*!
 * \brief Writes `value` in `base` into [`first`, `last`), as std::to_chars().
 *
 * A minus for a negative value, no plus, and digits beyond 9 in lower case; no
 * `'\0'` at the end.
 *
 * \code{.cpp}
 * char buffer[4];
 * utl::to_chars(buffer, buffer + 4, uint8_t{42});  // buffer + 2, "42"
 * \endcode
 *
 * \tparam T An integer other than `bool`; checked when compiled.
 * \param[out] first Where the text begins.
 * \param[in] last Where the room for it ends.
 * \param[in] value The number.
 * \param[in] base The base, 2 to 36.
 * \return The end of what was written, or CharconvError::kValueTooLarge if the
 *         text does not fit, or CharconvError::kInvalidArgument for a base not
 *         2 to 36. On an error, what lies in [`first`, `last`) is unspecified,
 *         as in the standard.
 *
 * \stddiff The base is a `uint8_t`, not an `int`: it lies in 2 to 36, and on
 *          the AVR an `int` takes two registers. A base not 2 to 36 gives an
 *          error where the standard leaves it undefined. No floating point: a
 *          correct to_chars() of a `double` would not fit on an Uno, and
 *          avr-libc has `dtostrf()`.
 */
template <typename T>
Result<char*> to_chars(char* first, char const* last, T value,
                       uint8_t base = internal::kDecimal) {
  static_assert(internal::IsCharconvInteger<T>::kValue,
                "to_chars: T is an integer other than bool; floating point is "
                "left out, see dtostrf() of avr-libc");
  if (!internal::IsBase(base)) {
    return CharconvError::kInvalidArgument;
  }
  bool const negative{internal::SignOf<T>::IsNegative(value)};
  char* digits{first};
  if (negative) {
    if (digits == last) {
      return CharconvError::kValueTooLarge;
    }
    *digits++ = '-';
  }
  return internal::WriteDigits(digits, last,
                               internal::Magnitude(value, negative), base);
}

/*!
 * \brief Writes `value` into `buffer`, as above, and returns the text written.
 *
 * The text is a view of the front of `buffer`, to print or to compare.
 *
 * \code{.cpp}
 * utl::array<char, 6> buffer;
 * utl::to_chars(buffer, -273);  // "-273"
 * \endcode
 *
 * \tparam T An integer other than `bool`.
 * \param[out] buffer Where to write.
 * \param[in] value The number.
 * \param[in] base The base, 2 to 36.
 * \return The text, or an error as above.
 */
template <typename T,
          // Not for to_chars(buffer, buffer + 8, value) of a char array, which
          // the overload above takes: else T would be char*, and the array a
          // span, and clang finds the two overloads equally good.
          typename = internal::EnableIf<!internal::IsPointer<T>::kValue>>
Result<string_view> to_chars(span<char> buffer, T value,
                             uint8_t base = internal::kDecimal) {
  char* const first{buffer.data()};
  Result<char*> const end{to_chars(first, first + buffer.size(), value, base)};
  if (!end.IsOk()) {
    return *end.error();
  }
  return string_view{first, static_cast<size_t>(end.value() - first)};
}

/*!
 * \brief Reads an integer in `base` from the front of [`first`, `last`) into
 *        `value`, as std::from_chars().
 *
 * It reads only what the standard reads: a minus for a signed T, no plus, no
 * space, no `0x`; digits beyond 9 in either case. It stops at the first char
 * that is not one of the number's digits.
 *
 * \tparam T An integer other than `bool`; checked when compiled.
 * \param[in] first Where the text begins.
 * \param[in] last Where it ends.
 * \param[out] value The number read; left as it is on an error.
 * \param[in] base The base, 2 to 36.
 * \return Where the number ends, which may be `last`.
 *         CharconvError::kInvalidArgument if no number begins at `first` or for
 *         a base not 2 to 36; CharconvError::kOutOfRange if the number does not
 *         fit into T.
 *
 * \stddiff On an error there is no pointer, where the standard gives one past
 *          the digits of a number out of range: a Result holds a value or an
 *          error, not both. To go on behind such a number, skip its digits. The
 *          base is a `uint8_t`, as for to_chars().
 */
template <typename T>
Result<char const*> from_chars(char const* first, char const* last, T& value,
                               uint8_t base = internal::kDecimal) {
  static_assert(internal::IsCharconvInteger<T>::kValue,
                "from_chars: T is an integer other than bool; floating point "
                "is left out");
  if (!internal::IsBase(base)) {
    return CharconvError::kInvalidArgument;
  }
  char const* digits{first};
  bool const negative{internal::SignOf<T>::ReadMinus(digits, last)};
  internal::UnsignedOf<T> magnitude{0};
  Result<char const*> const end{internal::ReadDigits(
      digits, last, base, internal::MagnitudeLimit<T>(negative), magnitude)};
  if (end.IsOk()) {
    value = internal::FromMagnitude<T>(magnitude, negative);
  }
  return end;
}

/*!
 * \brief Reads an integer from the front of `text`, as above, and returns it.
 *
 * `text` then begins after the number; on an error it is left as it is.
 *
 * \code{.cpp}
 * utl::string_view line{"42,7"};
 * utl::from_chars<uint8_t>(line);  // 42; line is ",7"
 * \endcode
 *
 * \tparam T The integer to read, named in the call.
 * \param[in,out] text The text to read from, shortened by what was read.
 * \param[in] base The base, 2 to 36.
 * \return The number, or an error as above.
 */
template <typename T>
Result<T> from_chars(string_view& text, uint8_t base = internal::kDecimal) {
  T value{};
  char const* const first{text.data()};
  Result<char const*> const end{
      from_chars(first, first + text.size(), value, base)};
  if (!end.IsOk()) {
    return *end.error();
  }
  text.remove_prefix(static_cast<size_t>(end.value() - first));
  return value;
}

}  // namespace utl

#endif  // UTL_SRC_UTL_CHARCONV_H_
