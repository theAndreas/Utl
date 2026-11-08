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
#include <utl/constant_wrapper.h>
#include <utl/flash_span.h>
#include <utl/function_ref.h>
#include <utl/inplace_function.h>
#include <utl/inplace_vector.h>
#include <utl/optional.h>
#include <utl/result.h>
#include <utl/span.h>
#include <utl/string_view.h>

#include <cstdint>

#include "test/beta_error.h"
#endif

namespace utl_test {
namespace {

using utl::Misuse;

#if UTL_CHECK_ACCESS
// A member function to bind to a pointer, and a function that takes one.
struct Switch {
  void Flip() { on = !on; }
  bool on{false};
};
void FlipIfAny(Switch* pointer) {
  if (pointer != nullptr) {
    pointer->Flip();
  }
}
#endif

std::string Printed(Misuse misuse) {
  StringPrint out;
  size_t const count{utl::PrintMisuse(out, misuse)};
  EXPECT_EQ(count, out.text.size());
  return out.text;
}

TEST(MisuseTest, PrintsEachKind) {
  EXPECT_EQ(Printed(Misuse::kResultValue),
            "utl: value() of a Result with an error");
  EXPECT_EQ(Printed(Misuse::kOptionalValue),
            "utl: * or -> of an empty optional");
  EXPECT_EQ(Printed(Misuse::kIndex), "utl: [] beyond the end");
  EXPECT_EQ(Printed(Misuse::kEmpty),
            "utl: front(), back() or pop_back() when empty");
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
  utl::span<int> const two{storage.data(), 2};
  utl::span<int> const none{storage.data() + 1, 0};
  char const text[] = "ab";
  utl::string_view const one{text, 1};

  EXPECT_EQ(Reported([] {
              utl::Result<int> const failed{BetaError::kFirst};
              static_cast<void>(failed.value());
            }),
            "utl: value() of a Result with an error\r\n");
  EXPECT_EQ(Reported([] {
              utl::optional<int> empty;
              static_cast<void>(*empty);
              utl::optional<int> const& same{empty};
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
            "utl: front(), back() or pop_back() when empty\r\n"
            "utl: front(), back() or pop_back() when empty\r\n");
  EXPECT_EQ(Reported([] {
              utl::string_view shortened{"ab"};
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
              utl::function_ref<void()> const f{none};
              static_cast<void>(f);
            }),
            "utl: function_ref of a null pointer\r\n");
  EXPECT_EQ(Reported([] {
              Switch* const volatile none{nullptr};
              utl::function_ref<void()> const f{UTL_CW(&Switch::Flip), none};
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
              size_t const volatile beyond{2};
              utl::array<int, 2> pair{{1, 2}};
              static_cast<void>(pair[beyond]);
            }),
            "utl: [] beyond the end\r\n");
}

// What follows each report is what the access does unchecked: the element
// read lies in the room of the vector, which is there all the same.
TEST(MisuseTest, ReportsAMisusedInplaceVector) {
  using Bytes = utl::inplace_vector<uint8_t, 2>;
  EXPECT_EQ(Reported([] {
              Bytes bytes{1};
              static_cast<void>(bytes[1]);
              Bytes const& const_bytes{bytes};
              static_cast<void>(const_bytes[1]);
            }),
            "utl: [] beyond the end\r\n"
            "utl: [] beyond the end\r\n");
  EXPECT_EQ(Reported([] {
              Bytes empty;
              Bytes const& const_empty{empty};
              static_cast<void>(empty.front());
              static_cast<void>(const_empty.front());
              static_cast<void>(empty.back());
              static_cast<void>(const_empty.back());
            }),
            "utl: front(), back() or pop_back() when empty\r\n"
            "utl: front(), back() or pop_back() when empty\r\n"
            "utl: front(), back() or pop_back() when empty\r\n"
            "utl: front(), back() or pop_back() when empty\r\n");
  EXPECT_EQ(Reported([] {
              Bytes empty;
              empty.pop_back();  // does nothing
              EXPECT_TRUE(empty.empty());
            }),
            "utl: front(), back() or pop_back() when empty\r\n");
  EXPECT_EQ(Reported([] {
              Bytes bytes{1, 2};
              // Past the last, and the first after the last: nothing removed.
              EXPECT_EQ(bytes.erase(bytes.end()), bytes.end());
              EXPECT_EQ(bytes.erase(bytes.begin() + 1, bytes.begin()),
                        bytes.end());
              EXPECT_EQ(bytes, (Bytes{1, 2}));
            }),
            "utl: [] beyond the end\r\n"
            "utl: [] beyond the end\r\n");
  EXPECT_EQ(Reported([] {
              Bytes const three{1, 2, 3};  // the first two are kept
              EXPECT_EQ(three, (Bytes{1, 2}));
            }),
            "utl: [] beyond the end\r\n");
}

// What follows each report is the read unchecked: of the array that is there
// all the same, as the views below see only part of it.
TEST(MisuseTest, ReportsAMisusedFlashSpan) {
  EXPECT_EQ(Reported([] {
              static constexpr utl::array<uint8_t, 2> kRows{{1, 2}};
              utl::flash_span<uint8_t> const one{kRows.data(), 1};
              // Empty, after the first: back() reads the one before.
              utl::flash_span<uint8_t> const none{kRows.data() + 1, 0};
              EXPECT_EQ(one[1], 2);
              EXPECT_EQ(none.front(), 2);
              EXPECT_EQ(none.back(), 1);
            }),
            "utl: [] beyond the end\r\n"
            "utl: front(), back() or pop_back() when empty\r\n"
            "utl: front(), back() or pop_back() when empty\r\n");
}

TEST(MisuseTest, RightAccessIsNotReported) {
  EXPECT_EQ(Reported([] {
              utl::array<int, 2> pair{{1, 2}};
              static_cast<void>(pair[1]);
              utl::Result<int> const ok{1};
              static_cast<void>(ok.value());
              utl::optional<int> const found{2};
              static_cast<void>(*found);
              int target{3};
              utl::optional<int&> const some{target};
              static_cast<void>(*some);
              utl::Result<int&> const ok_reference{target};
              static_cast<void>(ok_reference.value());
              auto const nothing = [] {};
              utl::function_ref<void()> const f{nothing};
              f();
              utl::function_ref<void()> const g{+nothing};  // a pointer
              g();
              utl::inplace_function<void()> const kept{nothing};
              kept();
              Switch light;
              utl::function_ref<void()> const flip{UTL_CW(&Switch::Flip),
                                                   &light};
              flip();
              // A function, not a member function, may take a null pointer.
              Switch* const none{nullptr};
              utl::function_ref<void()> const flip_any{UTL_CW(FlipIfAny), none};
              flip_any();
            }),
            "");
}
#endif

}  // namespace
}  // namespace utl_test
