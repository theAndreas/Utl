// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// flash_string_view, and how it meets a string_view. On the host flash is
// RAM, so these tests check the logic; that flash is read with pgm_read is
// what SelfCheck checks in simavr.

#include <Arduino.h>
#include <gtest/gtest.h>
#include <utl/flash_string_view.h>
#include <utl/index_error.h>
#include <utl/string_view.h>

namespace utl_test {
namespace {

using utl::flash_string_view;
using utl::IndexError;
using utl::string_view;

TEST(FlashStringViewTest, ReadsFlash) {
  flash_string_view const keyword{F("text=")};
  EXPECT_EQ(keyword.size(), 5U);
  EXPECT_EQ(keyword.length(), 5U);
  EXPECT_FALSE(keyword.empty());
  EXPECT_TRUE(flash_string_view{}.empty());
  EXPECT_EQ(keyword[4], '=');
  EXPECT_EQ(keyword.front(), 't');
  EXPECT_EQ(keyword.back(), '=');
  EXPECT_EQ(reinterpret_cast<char const*>(keyword.data())[0], 't');
  EXPECT_EQ((flash_string_view{F("text="), 4}), F("text"));
}

TEST(FlashStringViewTest, Iterates) {
  int ts{0};
  for (char const c : flash_string_view{F("text=")}) {
    if (c == 't') {
      ++ts;
    }
  }
  EXPECT_EQ(ts, 2);
}

TEST(FlashStringViewTest, SubstrFindAndAt) {
  flash_string_view const keyword{F("text=")};
  EXPECT_EQ(keyword.substr(0, 4).value(), F("text"));
  EXPECT_EQ(keyword.substr(1).value(), F("ext="));
  EXPECT_EQ(keyword.substr(6).status(), IndexError::kOutOfRange);
  EXPECT_EQ(keyword.find('='), 4U);
  EXPECT_EQ(keyword.find('t', 1), 3U);
  EXPECT_EQ(keyword.find('z'), flash_string_view::npos);
  EXPECT_EQ(keyword.find("xt"), 2U);
  EXPECT_EQ(keyword.find("t", 1), 3U);
  EXPECT_EQ(keyword.find("zz"), flash_string_view::npos);
  EXPECT_EQ(keyword.find("text=!"), flash_string_view::npos);
  EXPECT_EQ(keyword.at(0).ValueOr(0), 't');
  EXPECT_EQ(keyword.at(5).status(), IndexError::kOutOfRange);
}

TEST(FlashStringViewTest, ComparesWithFlash) {
  flash_string_view const keyword{F("text=")};
  EXPECT_EQ(keyword, F("text="));
  EXPECT_NE(keyword, F("text!"));
  EXPECT_NE(keyword, F("text"));
}

TEST(FlashStringViewTest, MeetsRam) {
  string_view const line{"text=Hi"};
  flash_string_view const keyword{F("text=")};
  EXPECT_TRUE(line.starts_with(keyword));
  EXPECT_FALSE(line.starts_with(F("clear")));
  EXPECT_FALSE(string_view{"tex"}.starts_with(keyword));
  EXPECT_TRUE(keyword.starts_with("tex"));
  EXPECT_FALSE(keyword.starts_with("text=Hi"));
  EXPECT_FALSE(keyword.starts_with("tez"));
  EXPECT_TRUE(line.ends_with(F("Hi")));
  EXPECT_FALSE(line.ends_with(F("Ho")));
  EXPECT_FALSE(string_view{"i"}.ends_with(F("Hi")));
  EXPECT_TRUE(keyword.ends_with("t="));
  EXPECT_FALSE(keyword.ends_with("text=!"));
  EXPECT_FALSE(keyword.ends_with("x="));
  EXPECT_TRUE(line.substr(5).value() == F("Hi"));
  EXPECT_TRUE(F("Hi") == line.substr(5).value());
  EXPECT_TRUE(line != F("text="));
  EXPECT_TRUE(F("text=") != line);
  EXPECT_FALSE(line == F("text=Ho"));
}

}  // namespace
}  // namespace utl_test
