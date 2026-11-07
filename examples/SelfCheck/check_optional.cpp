// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// optional<T> and optional<T&>.

#include <Utl.h>
#include <stddef.h>
#include <stdint.h>

#include "alpha_error.h"
#include "self_check.h"

namespace self_check {

namespace {

void CheckOptional() {
  utl::optional<uint8_t> const none{FindCode(42)};
  utl::optional<uint8_t> const found{FindCode(7)};

  Check(!none.HasValue() && !none && none == utl::nullopt &&
            utl::optional<uint8_t>{} == none,
        F("optional is empty"));
  Check(found.HasValue() && static_cast<bool>(found) && *found == 2 &&
            found == uint8_t{2} && found != none,
        F("optional holds a value"));

  PrintStatus(none.value().status());
  Check(found.value().ValueOr(0) == 2 &&
            none.value().status() == utl::OptionalError::kEmpty,
        F("optional::value returns a Result"));
  Check(found.ValueOr(9) == 2 && none.ValueOr(9) == 9, F("optional::ValueOr"));
  Check(none.OkOr(AlphaError::kSecond).status() == AlphaError::kSecond &&
            found.OkOr(AlphaError::kSecond).ValueOr(0) == 2,
        F("optional::OkOr"));

  utl::optional<uint8_t> changed{found};
  changed.reset();
  Check(!changed.HasValue(), F("optional::reset"));
  changed.emplace(5);
  Check(changed.HasValue() && *changed == 5 && changed != found,
        F("optional::emplace"));
  changed = utl::nullopt;
  Check(changed == none, F("optional takes nullopt"));
  changed.emplace(6);
  changed = {};
  Check(!changed.HasValue(), F("optional = {} is empty"));

  uint8_t calls{0};
  auto const find_again = [](uint8_t index) { return FindCode(index); };
  Check(FindCode(1).AndThen(find_again) == uint8_t{1} &&
            found.AndThen(find_again) == utl::nullopt,
        F("optional::AndThen"));
  Check(found.Map([](uint8_t index) { return index * 10; }) == 20 &&
            none.Map([&calls](uint8_t) {
              ++calls;
              return 1;
            }) == utl::nullopt &&
            calls == 0,
        F("optional::Map"));
  auto const three = [&calls] {
    ++calls;
    return utl::optional<uint8_t>{3};
  };
  Check(none.OrElse(three) == uint8_t{3} && calls == 1 &&
            found.OrElse(three) == uint8_t{2} && calls == 1,
        F("optional::OrElse"));
}

// The element of `rows` equal to `wanted`, or none: an optional of a
// reference, through which the caller writes.
utl::optional<uint8_t&> FindRow(utl::array<uint8_t, 3>& rows, uint8_t wanted) {
  for (uint8_t& row : rows) {
    if (row == wanted) {
      return row;
    }
  }
  return utl::nullopt;
}

void CheckOptionalReference() {
  utl::array<uint8_t, 3> rows{{codes[0], codes[1], codes[2]}};
  uint8_t spare{9};
  utl::optional<uint8_t&> const none{FindRow(rows, 42)};
  utl::optional<uint8_t&> const found{FindRow(rows, 7)};

  Check(!none && none == utl::nullopt && found && &*found == &rows[2],
        F("optional<T&> is empty or refers"));
  *found = 8;
  Check(rows[2] == 8 && found == uint8_t{8}, F("optional<T&> writes through"));
  utl::optional<uint8_t&> rebound{found};
  rebound = spare;
  Check(&*rebound == &spare && rows[2] == 8,
        F("optional<T&> rebinds when assigned"));
  Check(&none.ValueOr(spare) == &spare && &found.ValueOr(spare) == &rows[2],
        F("optional<T&>::ValueOr gives a reference"));
  Check(&found.value().value() == &rows[2] &&
            none.value().status() == utl::OptionalError::kEmpty,
        F("optional<T&>::value returns a Result<T&>"));
  utl::optional<bool> const mapped = found.Map([](uint8_t& row) {
    row = 7;
    return true;
  });
  Check(mapped == true && rows[2] == 7, F("optional<T&>::Map gets the T&"));
}

}  // namespace

void CheckOptionals() {
  CheckOptional();
  CheckOptionalReference();
}

}  // namespace self_check
