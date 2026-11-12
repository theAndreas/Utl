// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// What the checks of SelfCheck share: the report of each check, and the
// codes and lookups that several areas check against. One file per area,
// check_*.cpp, holds its checks and an entry that setup() calls in order.

#ifndef UTL_EXAMPLES_SELFCHECK_SELF_CHECK_H_
#define UTL_EXAMPLES_SELFCHECK_SELF_CHECK_H_

#include <Utl.h>
#include <stdint.h>

namespace self_check {

// Volatile, so that the compiler cannot fold the checks away.
extern utl::array<uint8_t volatile, 3> codes;

// Prints "pass <name>" or "FAIL <name>", and counts a failure.
void Check(bool passed, __FlashStringHelper const* name);

// The number of checks that failed so far.
uint8_t Failed();

// Prints `status` and ends the line.
void PrintStatus(utl::Status const& status);

// Returns codes[index], or BetaError::kFirst past the end.
utl::Result<uint8_t> GetCode(uint8_t index);

// Returns the index of the first code equal to `wanted`, or none.
utl::optional<uint8_t> FindCode(uint8_t wanted);

// The areas, in the order setup() calls them, which is the order of
// SelfCheck.expected.
void CheckErrors();            // check_error.cpp
void CheckResults();           // check_result.cpp
void CheckContainers();        // check_container.cpp
void CheckOptionals();         // check_optional.cpp
void CheckRustFluent();        // check_fluent.cpp
void CheckReferenceWrapper();  // check_reference_wrapper.cpp
void CheckFunctionRef();       // check_function_ref.cpp
void CheckInplaceFunction();   // check_inplace_function.cpp
void CheckText();              // check_text.cpp
void CheckFlashSpan();         // check_flash_span.cpp
void CheckSpscQueue();         // check_spsc_queue.cpp
void CheckBit();               // check_bit.cpp
void PushTick();               // check_spsc_queue.cpp, from its interrupt
void CheckMisuse();            // check_misuse.cpp

}  // namespace self_check

#endif  // UTL_EXAMPLES_SELFCHECK_SELF_CHECK_H_
