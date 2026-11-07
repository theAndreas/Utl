// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// inplace_function, as a callback that is kept above all: a lambda with its
// captures copied inside, without the heap.

#include <Utl.h>
#include <stdint.h>

#include "self_check.h"

namespace self_check {
namespace {

// A button that keeps what to call when pressed: the use it is made for.
class Button {
 public:
  void OnPress(utl::inplace_function<void()> const& handler) {
    on_press_ = handler;
  }
  void Press() const {
    if (on_press_) {
      on_press_();
    }
  }

 private:
  utl::inplace_function<void()> on_press_;
};

// Turns a count up when its button is pressed: a capture of this.
class Counter {
 public:
  explicit Counter(Button& button) {
    button.OnPress([this] { ++count_; });
  }
  uint8_t count() const { return count_; }

 private:
  uint8_t count_{0};
};

uint8_t Tripled(uint8_t value) { return static_cast<uint8_t>(3 * value); }

// Made in here, kept out there: the lambda is gone, its copy is not.
utl::inplace_function<uint8_t()> MakeCode(uint8_t index) {
  uint8_t const code{codes[index]};
  return [code] { return code; };
}

}  // namespace

void CheckInplaceFunction() {
  Button button;
  button.Press();  // nothing kept, nothing called
  Counter const counter{button};
  button.Press();
  button.Press();
  Check(counter.count() == 2, F("inplace_function keeps a lambda with this"));

  utl::inplace_function<uint8_t()> const code{MakeCode(2)};
  Check(code() == 7, F("inplace_function keeps a copy of its captures"));

  uint8_t count{codes[0]};
  utl::inplace_function<uint8_t()> first{[count]() mutable { return ++count; }};
  first();
  utl::inplace_function<uint8_t()> const copy{first};
  first = nullptr;
  Check(copy() == 2 && !first && count == 0,
        F("inplace_function copies its callable"));

  utl::inplace_function<uint8_t(uint8_t)> f{Tripled};
  uint8_t const tripled{f(codes[2])};
  f = nullptr;
  Check(tripled == 21 && !f, F("inplace_function of a function, and emptied"));
}

}  // namespace self_check
