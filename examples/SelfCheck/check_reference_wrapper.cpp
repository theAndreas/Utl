// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// reference_wrapper, as a member above all.

#include <Utl.h>
#include <stdint.h>

#include "self_check.h"

namespace self_check {
namespace {

// Counts into a counter another owns: a reference_wrapper as a member, what it
// is made for.
class Counter {
 public:
  explicit Counter(uint8_t& count) : count_{count} {}
  void Increment() { ++count_.get(); }

 private:
  utl::reference_wrapper<uint8_t> count_;
};

}  // namespace

void CheckReferenceWrapper() {
  uint8_t first{codes[1]};
  uint8_t second{codes[2]};
  // = rather than braces: what is checked is that an lvalue converts
  // implicitly.
  utl::reference_wrapper<uint8_t> wrapper = first;

  Check(&wrapper.get() == &first, F("reference_wrapper refers to its object"));
  wrapper.get() = 2;
  // = rather than braces: what is checked is that it converts to T& implicitly.
  uint8_t& converted = wrapper;
  Check(first == 2 && &converted == &first,
        F("reference_wrapper writes through get()"));
  wrapper = second;
  Check(&wrapper.get() == &second && first == 2,
        F("reference_wrapper rebinds when assigned"));

  Counter const counter{first};
  Counter copy{counter};
  copy.Increment();
  copy = Counter{second};
  copy.Increment();
  Check(first == 3 && second == 8,
        F("reference_wrapper as a member copies and assigns"));

  // = rather than braces: what is checked is that it converts implicitly.
  utl::reference_wrapper<uint8_t const> const read_only = utl::ref(first);
  Check(&read_only.get() == &first && &utl::cref(second).get() == &second,
        F("ref, cref, and to one of const"));

  utl::array<utl::reference_wrapper<uint8_t>, 2> const both{{first, second}};
  for (uint8_t& value : both) {
    value = 0;
  }
  Check(first == 0 && second == 0, F("reference_wrapper in an array"));
}

}  // namespace self_check
