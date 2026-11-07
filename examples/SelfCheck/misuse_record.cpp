// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "misuse_record.h"

#include <stdint.h>
#include <utl/config.h>  // IWYU pragma: keep
#include <utl/misuse.h>

namespace self_check {
namespace {

uint8_t count{0};
utl::Misuse last{utl::Misuse::kResultValue};

}  // namespace

uint8_t MisuseCount() { return count; }
utl::Misuse LastMisuse() { return last; }

}  // namespace self_check

#if UTL_CHECK_ACCESS
// Defined here rather than in SelfCheck.ino: arduino-cli writes a prototype
// for every function of an .ino, and one of utl::ReportMisuse() outside its
// namespace would not compile.
namespace utl {

void ReportMisuse(Misuse misuse) {
  ++self_check::count;
  self_check::last = misuse;
}

}  // namespace utl
#endif
