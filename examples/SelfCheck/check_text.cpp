// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// string_view, flash_string_view, and charconv.

#include <Utl.h>
#include <stddef.h>
#include <stdint.h>

#include "self_check.h"

namespace self_check {

using utl::IndexError;
using utl::Result;

namespace {

void CheckStringView() {
  char buffer[] = "text=Hi";
  utl::string_view const line{buffer};

  Check(line.size() == 7 && line == "text=Hi" && line != "text=Ho",
        F("string_view of a string"));
  Check(line.starts_with("text=") && !line.starts_with("texts") &&
            line.ends_with("Hi") && !line.ends_with("text=Hi!"),
        F("string_view::starts_with, ends_with"));
  Check(line.find('=') == 4 && line.find("Hi") == 5 &&
            line.find('x', 4) == utl::string_view::npos &&
            line.find("Ho") == utl::string_view::npos,
        F("string_view::find"));
  Check(line.substr(5).ValueOr("") == "Hi" && line.substr(7).value().empty() &&
            line.substr(8).status() == IndexError::kOutOfRange &&
            line.substr(1, 3).ValueOr("") == "ext",
        F("string_view::substr"));
  Check(line.at(0).ValueOr(0) == 't' &&
            line.at(7).status() == IndexError::kOutOfRange,
        F("string_view::at"));
}

void CheckFlashStringView() {
  char buffer[] = "text=Hi";
  utl::string_view const line{buffer};
  utl::flash_string_view const keyword{F("text=")};

  Check(keyword.size() == 5 && keyword[4] == '=' && keyword.front() == 't' &&
            keyword.back() == '=',
        F("flash_string_view reads flash"));
  Check(line.starts_with(keyword) && !line.starts_with(F("clear")) &&
            keyword.starts_with("tex") && !keyword.starts_with("text=Hi"),
        F("starts_with between RAM and flash"));
  Check(line.substr(5).value() == F("Hi") &&
            F("Hi") == line.substr(5).value() && line != F("text=") &&
            F("text=") != line,
        F("== between RAM and flash"));
  Check(keyword.substr(0, 4).value() == F("text") &&
            keyword.substr(6).status() == IndexError::kOutOfRange,
        F("flash_string_view::substr"));
  Check(keyword.find('=') == 4 &&
            keyword.find('z') == utl::flash_string_view::npos &&
            keyword.at(5).status() == IndexError::kOutOfRange,
        F("flash_string_view::find, at"));
  uint8_t ts{0};
  for (char const c : keyword) {
    if (c == 't') {
      ++ts;
    }
  }
  Check(ts == 2, F("flash_string_view in a range-based for"));
}

void CheckTrimAndCompare() {
  char buffer[] = "text=Hi";
  utl::string_view const line{buffer};
  utl::flash_string_view const keyword{F("text=")};

  utl::string_view trimmed{line};
  trimmed.remove_prefix(5);
  Check(trimmed == "Hi", F("string_view::remove_prefix"));
  trimmed = line;
  trimmed.remove_suffix(3);
  Check(trimmed == "text", F("string_view::remove_suffix"));
  Check(utl::string_view{"abc"} < "abd" && utl::string_view{"ab"} < "abc" &&
            utl::string_view{"b"} > "abc" && line.compare("text=Hi") == 0 &&
            utl::string_view{"ab"} <= "ab" && utl::string_view{"b"} >= "a",
        F("string_view::compare and <"));
  Check(utl::string_view{"\xe4"} > "z", F("string_view compares unsigned"));
  Check(keyword.ends_with("t=") && !keyword.ends_with("text=!") &&
            line.ends_with(F("Hi")) && !line.ends_with(F("Ho")),
        F("ends_with between RAM and flash"));
  Check(keyword.find("xt") == 2 &&
            keyword.find("zz") == utl::flash_string_view::npos &&
            keyword.find("text=!") == utl::flash_string_view::npos,
        F("flash_string_view::find of a text"));
}

void CheckCharconv() {
  utl::array<char, 8> buffer{};
  Check(utl::to_chars(buffer, uint8_t{codes[2]}).ValueOr("") == "7" &&
            utl::to_chars(buffer, int16_t{-32768}).ValueOr("") == "-32768" &&
            utl::to_chars(buffer, uint8_t{255}, 16).ValueOr("") == "ff",
        F("to_chars"));
  Check(utl::to_chars(buffer, int32_t{-12345678}).status() ==
            utl::CharconvError::kValueTooLarge,
        F("to_chars beyond the buffer"));

  utl::string_view line{"42,-7"};
  Check(utl::from_chars<uint8_t>(line).ValueOr(0) == 42 && line == ",-7",
        F("from_chars takes what it reads"));
  line.remove_prefix(1);
  Check(utl::from_chars<int8_t>(line).ValueOr(0) == -7 && line.empty(),
        F("from_chars of a negative"));
  utl::string_view large{"256"};
  Result<uint8_t> const out_of_range{utl::from_chars<uint8_t>(large)};
  PrintStatus(out_of_range.status());
  Check(out_of_range.status() == utl::CharconvError::kOutOfRange &&
            large == "256",
        F("from_chars out of range"));
}

}  // namespace

void CheckText() {
  CheckStringView();
  CheckFlashStringView();
  CheckTrimAndCompare();
  CheckCharconv();
}

}  // namespace self_check
