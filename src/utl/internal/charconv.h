// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The steps of to_chars() and from_chars(), each a function of its own;
 *        not part of the API.
 *
 * The integer and its sign, the digits written or read, and the limit that a
 * number read must not pass.
 */

#ifndef UTL_SRC_UTL_INTERNAL_CHARCONV_H_
#define UTL_SRC_UTL_INTERNAL_CHARCONV_H_

#include <stddef.h>
#include <stdint.h>

#include "utl/charconv_error.h"
#include "utl/result.h"

namespace utl {
namespace internal {

// Whether T is an integer that charconv takes: not bool, as in the standard,
// and no floating point, where 1 / 2 is not 0.
template <typename T>
struct IsCharconvInteger {
  static constexpr bool kValue{static_cast<T>(1) / static_cast<T>(2) ==
                               static_cast<T>(0)};
};
template <>
struct IsCharconvInteger<bool> {
  static constexpr bool kValue{false};
};

/*
 * The unsigned integer of T's width, which a number is counted in: a uint8_t
 * is converted with 8-bit arithmetic, not with the 32-bit division that
 * Print::print() of an integer pulls in.
 */
template <size_t Size>
struct UnsignedOfSizeImpl;
template <>
struct UnsignedOfSizeImpl<sizeof(uint8_t)> {
  using Type = uint8_t;
};
template <>
struct UnsignedOfSizeImpl<sizeof(uint16_t)> {
  using Type = uint16_t;
};
template <>
struct UnsignedOfSizeImpl<sizeof(uint32_t)> {
  using Type = uint32_t;
};
template <>
struct UnsignedOfSizeImpl<sizeof(uint64_t)> {
  using Type = uint64_t;
};
template <typename T>
using UnsignedOf = typename UnsignedOfSizeImpl<sizeof(T)>::Type;

// Whether T is signed; -1 < 1 rather than -1 < 0, which GCC takes for an
// unsigned compared with 0 and warns about.
template <typename T>
struct IsSigned {
  static constexpr bool kValue{static_cast<T>(-1) < static_cast<T>(1)};
};

// The minus, for a signed T; Sign<false> has none, so an unsigned T has no
// branch for it at all.
template <bool Signed>
struct Sign {
  template <typename T>
  static constexpr bool IsNegative(T value) {
    return value < static_cast<T>(0);
  }

  // Whether a minus lies at `in`; if so, `in` moves past it.
  static bool ReadMinus(char const*& in, char const* last) {
    if (in == last || *in != '-') {
      return false;
    }
    ++in;
    return true;
  }
};
template <>
struct Sign<false> {
  template <typename T>
  static constexpr bool IsNegative(T /*value*/) {
    return false;
  }
  static bool ReadMinus(char const*& /*in*/, char const* /*last*/) {
    return false;
  }
};
template <typename T>
using SignOf = Sign<IsSigned<T>::kValue>;

// The base to_chars() and from_chars() take when given none, as in the
// standard.
constexpr uint8_t kDecimal{10};

// The bases to_chars() and from_chars() take, as in the standard: from binary
// to the ten digits and the 26 letters.
constexpr uint8_t kLowestBase{2};
constexpr uint8_t kHighestBase{36};

constexpr bool IsBase(uint8_t base) {
  return base >= kLowestBase && base <= kHighestBase;
}

// The magnitude of `value`, modulo 2^n, so that the lowest value of a
// signed T, whose magnitude no T holds, is right as well.
template <typename T>
UnsignedOf<T> Magnitude(T value, bool negative) {
  using U = UnsignedOf<T>;
  U const bits{static_cast<U>(value)};
  return negative ? static_cast<U>(static_cast<U>(0) - bits) : bits;
}

// The T of a magnitude and a sign, modulo 2^n back, which GCC defines.
template <typename T>
T FromMagnitude(UnsignedOf<T> magnitude, bool negative) {
  using U = UnsignedOf<T>;
  return static_cast<T>(negative ? static_cast<U>(static_cast<U>(0) - magnitude)
                                 : magnitude);
}

// The largest magnitude a T takes: of its highest value, or, if `negative`,
// of its lowest, which for a signed T is one more.
template <typename T>
UnsignedOf<T> MagnitudeLimit(bool negative) {
  using U = UnsignedOf<T>;
  U const all{static_cast<U>(~U{0})};
  U const highest{IsSigned<T>::kValue ? static_cast<U>(all / 2U) : all};
  return negative ? static_cast<U>(highest + 1) : highest;
}

// The char of a digit below 36, in lower case as in the standard.
inline char DigitChar(uint8_t digit) {
  constexpr uint8_t kTen{10};
  return static_cast<char>(digit < kTen ? '0' + digit : 'a' + (digit - kTen));
}

// The value of a digit char in either case, or 36, which no base takes.
inline uint8_t DigitValue(char c) {
  constexpr uint8_t kTen{10};
  constexpr uint8_t kNone{36};
  if (c >= '0' && c <= '9') {
    return static_cast<uint8_t>(c - '0');
  }
  if (c >= 'a' && c <= 'z') {
    return static_cast<uint8_t>(c - 'a' + kTen);
  }
  if (c >= 'A' && c <= 'Z') {
    return static_cast<uint8_t>(c - 'A' + kTen);
  }
  return kNone;
}

// Reverses [first, last), the digits that WriteDigits() writes lowest first.
inline void Reverse(char* first, char* last) {
  while (first < last) {
    --last;
    char const c{*first};
    *first = *last;
    *last = c;
    ++first;
  }
}

/*
 * Writes the digits of `magnitude` in `base` from `first` on, and returns
 * their end, or CharconvError::kValueTooLarge at `last`. Each digit takes a
 * division in the width of U; written lowest first and then turned around,
 * they need no buffer of their own.
 */
template <typename U>
Result<char*> WriteDigits(char* first, char const* last, U magnitude,
                          uint8_t base) {
  U const radix{base};
  char* out{first};
  // At least one digit: 0 is written as "0".
  do {  // NOLINT(cppcoreguidelines-avoid-do-while)
    if (out == last) {
      return CharconvError::kValueTooLarge;
    }
    U const rest{static_cast<U>(magnitude / radix)};
    *out++ = DigitChar(static_cast<uint8_t>(magnitude - (rest * radix)));
    magnitude = rest;
  } while (magnitude != 0);
  Reverse(first, out);
  return out;
}

/*
 * Appends digits to a magnitude in a base, up to a limit. The limit is
 * divided once, here, as strtol() does: a magnitude beyond `cutoff_`, or at
 * it with a digit beyond `cutlim_`, would pass it, and no digit needs a
 * division of its own.
 */
template <typename U>
class DigitAppender {
 public:
  DigitAppender(uint8_t base, U limit)
      : radix_{base},
        cutoff_{static_cast<U>(limit / radix_)},
        cutlim_{static_cast<U>(limit - (cutoff_ * radix_))} {}

  // Appends `digit` to `magnitude`, or returns false if that passed the limit.
  bool Append(U& magnitude, uint8_t digit) const {
    if (magnitude > cutoff_ || (magnitude == cutoff_ && digit > cutlim_)) {
      return false;
    }
    magnitude = static_cast<U>((magnitude * radix_) + digit);
    return true;
  }

 private:
  U radix_;
  U cutoff_;
  U cutlim_;
};

/*
 * Reads the digits in `base` from `first` on into `magnitude`, up to `limit`,
 * and returns where they end: CharconvError::kInvalidArgument if there is
 * none, kOutOfRange if they pass the limit.
 */
template <typename U>
// Five parameters, one over FN-3. Base and limit handed in as the
// DigitAppender they make, the struct FN-3 asks for, cost SelfCheck 64 bytes
// of flash: the appender then lies in memory rather than in registers.
// Internal, with one caller, from_chars(), whose tests read every value of
// the 8- and 16-bit types in the bases 2, 10, 16 and 36.
// NOLINTNEXTLINE(readability-function-size)
Result<char const*> ReadDigits(char const* first, char const* last,
                               uint8_t base, U limit, U& magnitude) {
  DigitAppender<U> const appender{base, limit};
  char const* in{first};
  for (; in != last; ++in) {
    uint8_t const digit{DigitValue(*in)};
    if (digit >= base) {
      break;
    }
    if (!appender.Append(magnitude, digit)) {
      return CharconvError::kOutOfRange;
    }
  }
  if (in == first) {
    return CharconvError::kInvalidArgument;
  }
  return in;
}

}  // namespace internal
}  // namespace utl

#endif  // UTL_SRC_UTL_INTERNAL_CHARCONV_H_
