// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utl/messages.h"

#include <avr/pgmspace.h>
#include <stdint.h>

#include "utl/error_group.h"

namespace utl {
namespace {

// The groups whose texts are on, the last one turned on first.
Messages* first_messages{nullptr};

}  // namespace

__FlashStringHelper const* Messages::Find(ErrorGroup group, uint8_t code) {
  for (Messages const* messages{first_messages}; messages != nullptr;
       messages = messages->next_) {
    if (messages->group_ == group) {
      if (code >= messages->count_) {
        return nullptr;
      }
      return static_cast<__FlashStringHelper const*>(
          pgm_read_ptr(&messages->texts_[code]));
    }
  }
  return nullptr;
}

void UseMessages(Messages& messages) {
  for (Messages const* on{first_messages}; on != nullptr; on = on->next_) {
    if (on == &messages) {
      return;
    }
  }
  messages.next_ = first_messages;
  first_messages = &messages;
}

}  // namespace utl
