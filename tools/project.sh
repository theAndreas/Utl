#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# What the scripts in tools/ need to know of this project. tools/common.sh sources it after
# its own defaults, so what is defined here wins. The template wrote this file once; it is the
# project's, and `copier update` leaves it alone. What it may define:
#
#   NAMESPACE            the library's namespace; tools/docs.sh lets gaps in
#                        NAMESPACE::internal pass
#   FQBN                 the board the examples are compiled for
#   ARDUINO_CORE         the core, pinned, as arduino-cli installs it
#   ARDUINO_BOARD_URLS   the index of a third-party core, for arduino-cli core update-index
#   SIMAVR_MCU           what simavr runs examples/SelfCheck as; empty: no run in simavr
#   SIMAVR_FREQUENCY     its clock, in Hz
#   DATABASE_EXAMPLE     the example whose compiler flags tools/tidy.sh takes for the rest
#   SELF_CHECK_EXAMPLE   the example tools/self_check.sh runs
#   VARIANTS             the builds every example is compiled in, see variant_flags()
#   TIDY_VARIANTS        the variants clang-tidy and cppcheck read: those whose defines change
#                        the code. The first is named default: clangd reads its database
#   DOCS_INTERNAL_API    a regex of the files in NAMESPACE::internal that are API after all,
#                        such as a base a public class takes members from
#   CPPCHECK             0 if cppcheck cannot parse the library, with the reason; else 1
#
#   variant_flags VARIANT   prints the compiler flags of a variant, one per line
#   prepare                 builds what compiling needs beyond the core, such as a plugin
#   check_elf VARIANT ELF   fails if a compiled example is wrong in a way no warning says

# shellcheck disable=SC2034  # read by the scripts that source this file

NAMESPACE="utl"
FQBN="arduino:avr:uno"
# Pinned: the plugin loads only into the cc1plus it was built for, and the sizes in the README
# were measured with it.
ARDUINO_CORE="arduino:avr@1.8.8"
SIMAVR_MCU="atmega328p"
SIMAVR_FREQUENCY="16000000"
# Status and Result give a sketch the functions of their base in internal/fluent.h.
DOCS_INTERNAL_API='^src/utl/internal/fluent\.h:'
# cppcheck 2.19 stops at a syntax error in src/utl/array.h:181, the pack expansion in
# `internal::Void<decltype(T{internal::DeclVal<Ts>()})...>`, which GCC and Clang compile; and a
# syntax error ends the analysis of every translation unit that includes it, which is all.
CPPCHECK=0

readonly PLUGIN="${BUILD_DIR}/avr-flash-vtbl.so"

# Every example in three variants: "ram" (vtables in RAM, the default of avr-gcc), "flash"
# (vtables in flash, with the plugin avr-flash-vtbl) and "checked" (vtables in RAM, misuse
# reported with UTL_CHECK_ACCESS=1). utl itself has no vtable since its groups lost their
# virtual functions; "flash" still moves the core's. clang-tidy reads the two whose defines
# differ.
VARIANTS=(ram flash checked)
TIDY_VARIANTS=(default checked)

variant_flags() {
  case "$1" in
    # extra_flags reach every C++ file of the build, the core included. A file compiled
    # without the plugin would read a vtable in flash from RAM.
    flash) echo "-fplugin=${PLUGIN}" ;;
    checked) echo "-DUTL_CHECK_ACCESS=1" ;;
  esac
}

# Builds the plugin, against the avr-gcc of the core; again after the core is updated.
prepare() {
  "${ROOT_DIR}/tools/build_plugin.sh"
}

# Fails if a vtable of the flash variant lies in .data, that is, in RAM: then some file was
# compiled without the plugin, and a virtual call through it would jump anywhere.
check_elf() {
  local variant="$1"
  local elf="$2"
  if [[ "${variant}" != "flash" ]]; then
    return 0
  fi
  local in_ram
  in_ram="$("$(avr_gcc_bin_dir)/avr-nm" -C "${elf}" \
    | grep ' [dD] vtable for ' || true)"
  if [[ -n "${in_ram}" ]]; then
    err "vtables in RAM despite the plugin, in ${elf}:"
    err "${in_ram}"
    return 1
  fi
}
