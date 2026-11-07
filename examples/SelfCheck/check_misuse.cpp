// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The report of a misuse, with UTL_CHECK_ACCESS at 1; without, nothing.

#include <Utl.h>

#include "self_check.h"

// What only the checks with the switch on use.
#if UTL_CHECK_ACCESS
#include <stdint.h>

#include "beta_error.h"
#include "misuse_record.h"
#endif

namespace self_check {

#if UTL_CHECK_ACCESS
namespace {

// A member function to bind to a null pointer, never called.
struct Probe {
  uint8_t Read() const { return value; }
  uint8_t value{0};
};

}  // namespace
#endif

// With UTL_CHECK_ACCESS at 1, checks that each misuse is reported once, and a
// right access not at all. The reads beyond the end and of an empty span are
// undefined, but harmless on the AVR, where they read a byte of RAM.
void CheckMisuse() {
#if UTL_CHECK_ACCESS
  using utl::Result;
  using utl::span;
  uint8_t before{MisuseCount()};
  Result<uint8_t> const failed_result{GetCode(9)};
  static_cast<void>(failed_result.value());
  Check(
      MisuseCount() == before + 1 && LastMisuse() == utl::Misuse::kResultValue,
      F("value() of a Result with an error is reported"));
  before = MisuseCount();
  utl::optional<uint8_t> const none;
  static_cast<void>(*none);
  Check(MisuseCount() == before + 1 &&
            LastMisuse() == utl::Misuse::kOptionalValue,
        F("* of an empty optional is reported"));
  utl::array<uint8_t, 2> pair{{1, 2}};
  before = MisuseCount();
  static_cast<void>(pair[2]);
  Check(MisuseCount() == before + 1 && LastMisuse() == utl::Misuse::kIndex,
        F("[] beyond the end is reported"));
  span<uint8_t> const empty;
  before = MisuseCount();
  static_cast<void>(empty.front());
  Check(MisuseCount() == before + 1 && LastMisuse() == utl::Misuse::kEmpty,
        F("front() of an empty span is reported"));
  utl::optional<uint8_t&> const no_row;
  before = MisuseCount();
  static_cast<void>(*no_row);
  Check(MisuseCount() == before + 1 &&
            LastMisuse() == utl::Misuse::kOptionalValue,
        F("* of an empty optional<T&> is reported"));
  Result<uint8_t&> const no_reference{BetaError::kFirst};
  before = MisuseCount();
  static_cast<void>(no_reference.value());
  Check(
      MisuseCount() == before + 1 && LastMisuse() == utl::Misuse::kResultValue,
      F("value() of a Result<T&> with an error is reported"));

  utl::string_view shortened{"ab"};
  before = MisuseCount();
  shortened.remove_prefix(3);
  Check(MisuseCount() == before + 1 && LastMisuse() == utl::Misuse::kRemove &&
            shortened.empty(),
        F("remove_prefix() beyond the end is reported, and empties"));

  // Made, never called: a call would jump to address 0 and restart.
  void (*const volatile no_function)() = nullptr;
  before = MisuseCount();
  utl::function_ref<void()> const null_ref{no_function};
  static_cast<void>(null_ref);
  Check(
      MisuseCount() == before + 1 && LastMisuse() == utl::Misuse::kNullFunction,
      F("a function_ref of a null pointer is reported"));
  Probe* const volatile no_probe{nullptr};
  before = MisuseCount();
  utl::function_ref<uint8_t()> const null_object{UTL_CW(&Probe::Read),
                                                 no_probe};
  static_cast<void>(null_object);
  Check(
      MisuseCount() == before + 1 && LastMisuse() == utl::Misuse::kNullFunction,
      F("a function_ref of a member and a null object is reported"));
  utl::inplace_vector<uint8_t, 2> no_codes;
  before = MisuseCount();
  no_codes.pop_back();
  Check(MisuseCount() == before + 1 && LastMisuse() == utl::Misuse::kEmpty &&
            no_codes.empty(),
        F("pop_back() of an empty inplace_vector is reported, and does "
          "nothing"));
  utl::inplace_function<uint8_t()> const no_callback;
  before = MisuseCount();
  uint8_t const placeholder{no_callback()};
  Check(MisuseCount() == before + 1 &&
            LastMisuse() == utl::Misuse::kEmptyFunction && placeholder == 0,
        F("a call of an empty inplace_function is reported, and gives 0"));

  before = MisuseCount();
  static_cast<void>(pair[1]);
  static_cast<void>(GetCode(1).value());
  static_cast<void>(*FindCode(7));
  Check(MisuseCount() == before, F("a right access is not reported"));

  utl::PrintMisuse(Serial, utl::Misuse::kIndex);
  Serial.println();
  utl::PrintMisuse(Serial, utl::Misuse::kRemove);
  Serial.println();
  utl::PrintMisuse(Serial, utl::Misuse::kNullFunction);
  Serial.println();
  utl::PrintMisuse(Serial, utl::Misuse::kEmptyFunction);
  Serial.println();
#endif
}

}  // namespace self_check
