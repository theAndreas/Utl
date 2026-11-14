// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// What each type takes on the Uno, and what is constexpr: checked when
// compiled, so a change that grows a type stops the build of SelfCheck.

#include <Utl.h>
#include <stdint.h>

namespace self_check {
namespace {

using utl::Error;
using utl::Result;
using utl::span;
using utl::Status;

static_assert(sizeof(Error) == 3, "an Error is a pointer and a byte");
static_assert(sizeof(Status) == 3,
              "a Status is an Error, with a null group if ok");
static_assert(sizeof(Result<uint8_t>) == 4, "a Result is a value and a Status");
static_assert(sizeof(Result<void>) == 3, "a Result<void> is a Status");
static_assert(sizeof(utl::array<uint8_t, 3>) == 3, "an array is its elements");
static_assert(sizeof(span<uint8_t>) == 4, "a span is a pointer and a size");
static_assert(sizeof(utl::optional<uint8_t>) == 2,
              "an optional is a value and a flag");
static_assert(sizeof(utl::optional<uint8_t&>) == 2,
              "an optional of a reference is a pointer");
static_assert(sizeof(Result<uint8_t&>) == 5,
              "a Result of a reference is a pointer and a Status");
static_assert(sizeof(utl::reference_wrapper<uint8_t>) == 2,
              "a reference_wrapper is a pointer");
static_assert(sizeof(utl::function_ref<void(uint8_t)>) == 4,
              "a function_ref is two pointers");
static_assert(sizeof(utl::inplace_function<void(uint8_t)>) == 8,
              "an inplace_function is 4 bytes for the callable, two pointers");
static_assert(sizeof(utl::inplace_vector<uint8_t, 8>) == 9,
              "an inplace_vector is its room and a count of 1 byte");
static_assert(sizeof(utl::string_view) == 4 &&
                  sizeof(utl::flash_string_view) == 4,
              "a string_view is a pointer and a size");
constexpr utl::array<uint8_t, 3> kConstantRows{{1, 2, 3}};
static_assert(kConstantRows[1] == 2 && kConstantRows.size() == 3,
              "an array is constexpr");
constexpr auto kMadeRows = utl::make_array(uint8_t{1}, uint8_t{2}, uint8_t{3});
static_assert(kMadeRows.size() == 3 && kMadeRows[2] == 3 &&
                  sizeof(kMadeRows) == 3,
              "make_array takes type and size from its arguments");
constexpr auto kTypedRows = utl::make_array<uint8_t>(1, 2, 255);
static_assert(kTypedRows.size() == 3 && kTypedRows[2] == 255 &&
                  sizeof(kTypedRows) == 3,
              "make_array<uint8_t> takes the size from its values");

}  // namespace
}  // namespace self_check
