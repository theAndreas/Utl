// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utl/messages.h"

#include <avr/pgmspace.h>
#include <stdint.h>

#include "utl/error_group.h"

namespace utl {
namespace {

// The groups whose texts are on, the last one turned on first. A sketch turns
// them on at run time, so the head of the list cannot be const.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
Messages* first_messages{nullptr};

}  // namespace

__FlashStringHelper const* Messages::Find(ErrorGroup group, uint8_t code) {
  Messages const* messages{first_messages};
  while (messages != nullptr) {
    if (messages->group_ == group) {
      if (code >= messages->count_) {
        return nullptr;
      }
      return static_cast<__FlashStringHelper const*>(
          pgm_read_ptr(&messages->texts_[code]));
    }
    messages = messages->next_;
  }
  return nullptr;
}

void Messages::Use() {
  Messages const* on{first_messages};
  while (on != nullptr) {
    if (on == this) {
      return;
    }
    on = on->next_;
  }
  next_ = first_messages;
  first_messages = this;
}

}  // namespace utl
