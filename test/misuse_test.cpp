// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The report of a misuse, with UTL_CHECK_ACCESS at 1: the default report,
// which prints on Serial, and PrintMisuse(). Without the switch only
// PrintMisuse() is tested, as nothing is reported.

#include <gtest/gtest.h>
// For UTL_CHECK_ACCESS, which only #if reads: include-cleaner takes that for
// no use.
#include <utl/config.h>  // IWYU pragma: keep
#include <utl/misuse.h>

#include <cstddef>
#include <string>

#include "test/string_print.h"

// What only the checks with the switch on use.
#if UTL_CHECK_ACCESS
#include <Arduino.h>
#include <utl/array.h>
#include <utl/function_ref.h>
#include <utl/inplace_function.h>
#include <utl/optional.h>
#include <utl/result.h>
#include <utl/span.h>
#include <utl/string_view.h>

#include "test/beta_error.h"
#endif

namespace utl_test {
namespace {

using utl::Misuse;

std::string Printed(Misuse misuse) {
  StringPrint out;
  size_t const count = utl::PrintMisuse(out, misuse);
  EXPECT_EQ(count, out.text.size());
  return out.text;
}

TEST(MisuseTest, PrintsEachKind) {
  EXPECT_EQ(Printed(Misuse::kResultValue),
            "utl: value() of a Result with an error");
  EXPECT_EQ(Printed(Misuse::kOptionalValue),
            "utl: * or -> of an empty optional");
  EXPECT_EQ(Printed(Misuse::kIndex), "utl: [] beyond the end");
  EXPECT_EQ(Printed(Misuse::kEmpty), "utl: front() or back() of an empty view");
  EXPECT_EQ(Printed(Misuse::kRemove),
            "utl: remove_prefix() or remove_suffix() beyond the end");
  EXPECT_EQ(Printed(Misuse::kNullFunction),
            "utl: function_ref of a null pointer");
  EXPECT_EQ(Printed(Misuse::kEmptyFunction),
            "utl: call of an empty inplace_function");
  // A value beyond the enum, which PrintMisuse() has to leave without a text.
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  auto const beyond = static_cast<Misuse>(200);
  EXPECT_EQ(Printed(beyond), "utl: ");
}

#if UTL_CHECK_ACCESS
// What the default report printed for one misuse.
template <typename F>
std::string Reported(F misuse) {
  Serial.output.clear();
  misuse();
  return Serial.output;
}

TEST(MisuseTest, ReportsEachMisuseOnSerial) {
  // The tests form each wrong access but never read it: static_cast<void> of a
  // reference reads nothing, so the sanitizers have nothing to stop. The views
  // point at memory that is there all the same, the array of three below.
  utl::array<int, 3> storage = {{1, 2, 3}};
  utl::span<int> const two(storage.data(), 2);
  utl::span<int> const none(storage.data() + 1, 0);
  char const text[] = "ab";
  utl::string_view const one(text, 1);

  EXPECT_EQ(Reported([] {
              utl::Result<int> const failed = BetaError::kFirst;
              static_cast<void>(failed.value());
            }),
            "utl: value() of a Result with an error\r\n");
  EXPECT_EQ(Reported([] {
              utl::optional<int> empty;
              static_cast<void>(*empty);
              utl::optional<int> const& same = empty;
              static_cast<void>(*same);
            }),
            "utl: * or -> of an empty optional\r\n"
            "utl: * or -> of an empty optional\r\n");
  // Of an optional<T&> only ->, which gives the null pointer and reads
  // nothing: * would bind a reference to it, which UBSan stops, and so would
  // value() of a Result<T&> with an error. SelfCheck checks both on the AVR.
  EXPECT_EQ(Reported([] {
              utl::optional<int&> const empty;
              static_cast<void>(empty.operator->());
            }),
            "utl: * or -> of an empty optional\r\n");
  EXPECT_EQ(Reported([&] {
              static_cast<void>(two[2]);
              static_cast<void>(one[1]);
            }),
            "utl: [] beyond the end\r\n"
            "utl: [] beyond the end\r\n");
  EXPECT_EQ(Reported([&] {
              static_cast<void>(none.front());
              static_cast<void>(none.back());
            }),
            "utl: front() or back() of an empty view\r\n"
            "utl: front() or back() of an empty view\r\n");
  EXPECT_EQ(Reported([] {
              utl::string_view shortened = "ab";
              shortened.remove_prefix(3);
              EXPECT_TRUE(shortened.empty());
              shortened = "ab";
              shortened.remove_suffix(3);
              EXPECT_TRUE(shortened.empty());
            }),
            "utl: remove_prefix() or remove_suffix() beyond the end\r\n"
            "utl: remove_prefix() or remove_suffix() beyond the end\r\n");
  // Made, never called: a call would jump to address 0.
  EXPECT_EQ(Reported([] {
              void (*const volatile none)() = nullptr;
              utl::function_ref<void()> const f = none;
              static_cast<void>(f);
            }),
            "utl: function_ref of a null pointer\r\n");
  EXPECT_EQ(Reported([] {
              utl::inplace_function<int()> const empty;
              EXPECT_EQ(empty(), 0);  // the default, after the report
            }),
            "utl: call of an empty inplace_function\r\n");
}

TEST(MisuseTest, ReportsAnArrayIndexBeyondTheEnd) {
  EXPECT_EQ(Reported([] {
              size_t const volatile beyond = 2;
              utl::array<int, 2> pair = {{1, 2}};
              static_cast<void>(pair[beyond]);
            }),
            "utl: [] beyond the end\r\n");
}

TEST(MisuseTest, RightAccessIsNotReported) {
  EXPECT_EQ(Reported([] {
              utl::array<int, 2> pair = {{1, 2}};
              static_cast<void>(pair[1]);
              utl::Result<int> const ok = 1;
              static_cast<void>(ok.value());
              utl::optional<int> const found = 2;
              static_cast<void>(*found);
              int target = 3;
              utl::optional<int&> const some = target;
              static_cast<void>(*some);
              utl::Result<int&> const ok_reference = target;
              static_cast<void>(ok_reference.value());
              auto const nothing = [] {};
              utl::function_ref<void()> const f = nothing;
              f();
              utl::function_ref<void()> const g = +nothing;  // a pointer
              g();
              utl::inplace_function<void()> const kept = nothing;
              kept();
            }),
            "");
}
#endif

}  // namespace
}  // namespace utl_test
