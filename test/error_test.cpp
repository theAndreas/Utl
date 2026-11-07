// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Error: a group and a code, and how it prints.

#include <gtest/gtest.h>
#include <utl/charconv_error.h>
#include <utl/error.h>
#include <utl/error_group.h>
#include <utl/index_error.h>
#include <utl/optional_error.h>

#include <cstddef>
#include <string>

#include "test/alpha_error.h"
#include "test/beta_error.h"
#include "test/string_print.h"

namespace utl_test {
namespace {

using utl::Error;
using utl::ErrorGroup;

constexpr ErrorGroup kAlpha{kAlphaErrorGroup};
constexpr ErrorGroup kBeta{kBetaErrorGroup};

std::string Printed(Error const& error) {
  StringPrint out;
  size_t const count = error.PrintTo(out);
  EXPECT_EQ(count, out.text.size());
  return out.text;
}

// An error knows its group by address and its code.
TEST(ErrorTest, HasGroupAndCode) {
  Error const error = AlphaError::kSecond;
  EXPECT_EQ(error.group(), kAlpha);
  EXPECT_EQ(error.code(), 1);
}

// Equal only if group and code are; codes of two groups may overlap.
TEST(ErrorTest, ComparesGroupAndCode) {
  Error const alpha_first = AlphaError::kFirst;
  EXPECT_EQ(alpha_first, AlphaError::kFirst);
  EXPECT_EQ(AlphaError::kFirst, alpha_first);
  EXPECT_NE(alpha_first, AlphaError::kSecond);
  EXPECT_NE(alpha_first, BetaError::kFirst);
  EXPECT_EQ(Error(kBeta, 0), BetaError::kFirst);
}

// The group is its address: an error made in another file is equal.
TEST(ErrorTest, EqualAcrossTranslationUnits) {
  EXPECT_EQ(MakeAlphaSecondElsewhere(), AlphaError::kSecond);
}

TEST(ErrorTest, IsAssignable) {
  Error error = AlphaError::kFirst;
  error = BetaError::kFirst;
  EXPECT_EQ(error, BetaError::kFirst);
}

// A group whose texts are on says what went wrong.
TEST(ErrorTest, SaysWhatWentWrong) {
  UseAlphaMessages();
  EXPECT_STREQ(
      reinterpret_cast<char const*>(Error(AlphaError::kSecond).Message()),
      "alpha second");
  EXPECT_EQ(Printed(AlphaError::kFirst), "alpha: alpha first");
}

// A code beyond a group's table of texts prints as a code, and has no text.
TEST(ErrorTest, CodeBeyondTheTexts) {
  UseAlphaMessages();
  EXPECT_EQ(Printed(Error(kAlpha, 7)), "alpha: 7");
  EXPECT_STREQ(reinterpret_cast<char const*>(Error(kAlpha, 7).Message()), "");
}

// A group is the address of its name, not its words: another array that
// says "alpha" is another group.
TEST(ErrorTest, SameWordsAreAnotherGroup) {
  static char const kOtherAlpha[] = "alpha";
  EXPECT_NE(ErrorGroup(kOtherAlpha), kAlpha);
  EXPECT_NE(Error(ErrorGroup(kOtherAlpha), 0), AlphaError::kFirst);
}

// A group whose texts are off prints its code, every digit of it, and has an
// empty text.
TEST(ErrorTest, PrintsCodeWithoutTexts) {
  EXPECT_STREQ(
      reinterpret_cast<char const*>(Error(BetaError::kFirst).Message()), "");
  EXPECT_EQ(Printed(BetaError::kFirst), "beta: 0");
  EXPECT_EQ(Printed(Error(kBeta, 10)), "beta: 10");
  EXPECT_EQ(Printed(Error(kBeta, 105)), "beta: 105");
  EXPECT_EQ(Printed(Error(kBeta, 255)), "beta: 255");
}

// Turning the texts on twice keeps one entry: the list does not loop.
TEST(ErrorTest, TextsTurnedOnTwice) {
  UseAlphaMessages();
  UseAlphaMessages();
  EXPECT_EQ(Printed(AlphaError::kSecond), "alpha: alpha second");
  EXPECT_EQ(Printed(BetaError::kFirst), "beta: 0");
}

// The groups of the library itself, each turned on by a function of its own.
TEST(ErrorTest, LibraryGroupsSayWhatWentWrong) {
  utl::UseIndexMessages();
  utl::UseOptionalMessages();
  utl::UseCharconvMessages();
  EXPECT_EQ(Printed(utl::IndexError::kOutOfRange), "index: out of range");
  EXPECT_EQ(Printed(utl::IndexError::kFull), "index: full");
  EXPECT_EQ(Printed(utl::OptionalError::kEmpty), "optional: empty");
  EXPECT_EQ(Printed(utl::CharconvError::kInvalidArgument),
            "charconv: invalid argument");
  EXPECT_EQ(Printed(utl::CharconvError::kOutOfRange), "charconv: out of range");
  EXPECT_EQ(Printed(utl::CharconvError::kValueTooLarge),
            "charconv: value too large");
}

}  // namespace
}  // namespace utl_test
