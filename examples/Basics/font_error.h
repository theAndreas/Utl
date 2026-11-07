// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The errors of a font, one of two groups this example defines outside the
// library.

#ifndef UTL_EXAMPLES_BASICS_FONT_ERROR_H_
#define UTL_EXAMPLES_BASICS_FONT_ERROR_H_

#include <avr/pgmspace.h>
#include <stdint.h>
#include <utl/error_group.h>

namespace font {

enum class FontError : uint8_t {
  kUnknownChar,
  kNullText,
};

// "font", defined in font_error.cpp; its address is the group.
// A name in flash is a char array, declared here without its text, as only
// one file may define it.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
extern char const kFontErrorGroup[] PROGMEM;

constexpr utl::ErrorGroup GetErrorGroup(FontError) {
  return utl::ErrorGroup{kFontErrorGroup};
}

// Turns on the texts of the group: until then its errors print their codes.
void UseFontMessages();

}  // namespace font

#endif  // UTL_EXAMPLES_BASICS_FONT_ERROR_H_
