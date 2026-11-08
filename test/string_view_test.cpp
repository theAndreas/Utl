// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// string_view: a view of text in RAM.

#include <gtest/gtest.h>
#include <utl/index_error.h>
#include <utl/string_view.h>

namespace utl_test {
namespace {

using utl::IndexError;
using utl::string_view;

TEST(StringViewTest, OfAString) {
  string_view const line{"text=Hi"};
  EXPECT_EQ(line.size(), 7U);
  EXPECT_EQ(line.length(), 7U);
  EXPECT_FALSE(line.empty());
  EXPECT_TRUE(string_view{}.empty());
  EXPECT_EQ(line, "text=Hi");
  EXPECT_NE(line, "text=Ho");
  EXPECT_NE(line, "text");
  EXPECT_EQ(line[0], 't');
  EXPECT_EQ(line.front(), 't');
  EXPECT_EQ(line.back(), 'i');
  EXPECT_EQ(line.end() - line.begin(), 7);
  EXPECT_EQ((string_view{line.data(), 4}), "text");
}

TEST(StringViewTest, StartsAndEndsWith) {
  string_view const line{"text=Hi"};
  EXPECT_TRUE(line.starts_with("text="));
  EXPECT_FALSE(line.starts_with("texts"));
  EXPECT_FALSE(line.starts_with("text=Hi!"));
  EXPECT_TRUE(line.ends_with("Hi"));
  EXPECT_FALSE(line.ends_with("Ho"));
  EXPECT_FALSE(line.ends_with("text=Hi!"));
}

TEST(StringViewTest, Find) {
  string_view const line{"text=Hi"};
  EXPECT_EQ(line.find('='), 4U);
  EXPECT_EQ(line.find('t', 1), 3U);
  EXPECT_EQ(line.find('x', 4), string_view::npos);
  EXPECT_EQ(line.find("Hi"), 5U);
  EXPECT_EQ(line.find("t", 1), 3U);
  EXPECT_EQ(line.find("Ho"), string_view::npos);
  EXPECT_EQ(line.find("text=Hi!"), string_view::npos);
}

TEST(StringViewTest, SubstrAndAt) {
  string_view const line{"text=Hi"};
  EXPECT_EQ(line.substr(5).ValueOr(""), "Hi");
  EXPECT_EQ(line.substr(1, 3).ValueOr(""), "ext");
  EXPECT_TRUE(line.substr(7).value().empty());
  EXPECT_EQ(line.substr(8).status(), IndexError::kOutOfRange);
  EXPECT_EQ(line.at(0).ValueOr(0), 't');
  EXPECT_EQ(line.at(7).status(), IndexError::kOutOfRange);
}

TEST(StringViewTest, RemovePrefixAndSuffix) {
  string_view const line{"text=Hi"};
  string_view trimmed{line};
  trimmed.remove_prefix(5);
  EXPECT_EQ(trimmed, "Hi");
  trimmed = line;
  trimmed.remove_suffix(3);
  EXPECT_EQ(trimmed, "text");
}

TEST(StringViewTest, CompareOrdersAsUnsignedChar) {
  EXPECT_LT(string_view{"abc"}, "abd");
  EXPECT_LT(string_view{"ab"}, "abc");
  EXPECT_GT(string_view{"b"}, "abc");
  EXPECT_LE(string_view{"ab"}, "ab");
  EXPECT_GE(string_view{"b"}, "a");
  EXPECT_EQ(string_view{"ab"}.compare("ab"), 0);
  EXPECT_GT(string_view{"abc"}.compare("ab"), 0);
  EXPECT_GT(string_view{"\xe4"}, "z");
}

}  // namespace
}  // namespace utl_test
