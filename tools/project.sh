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
#   HOST_CODE            folders of code that runs on the host beside the library, as a
#                        simulator, built by CMakeLists.txt: tools/lint.sh formats and checks
#                        them as src/, tools/tidy.sh reads them with the tests
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

# Two targets, the examples built for the same Uno twice. board: by arduino-cli with arduino:avr's
# avr-gcc 7.3, as a sketch is built, which the sizes in the README measure. cmake: by CMake with
# avr-gcc 14 from apt, the core of arduino:avr compiled by CMakeLists.txt, as only a GCC of 10 or
# later writes the call graphs make stack adds up.
TARGETS=(board cmake)

target_settings() {
  case "$1" in
    cmake)
      BUILD_SYSTEM="cmake"
      CMAKE_TOOLCHAIN="cmake/toolchains/avr-gcc.cmake"
      CMAKE_DEFINES=(-DAVR_MCU="${SIMAVR_MCU}" -DAVR_F_CPU="${SIMAVR_FREQUENCY}"
        -DARDUINO_CORE_DIR="$(arduino_core_dir)" -DARDUINO_VARIANT=standard)
      ;;
  esac
}

# Prints where arduino-cli installs ARDUINO_CORE: packages/<vendor>/hardware/<arch>/<version> of
# its data folder. Nothing where arduino-cli is not there yet, as when CI asks tools/setup.sh which
# build systems the targets have before it installs arduino-cli; CMake then says what is missing.
arduino_core_dir() {
  local data
  data="$(arduino-cli config get directories.data 2> /dev/null)" || return 0
  local core="${ARDUINO_CORE%@*}"
  echo "${data}/packages/${core%%:*}/hardware/${core#*:}/${ARDUINO_CORE#*@}"
}

# The plugin of each avr-gcc: one loads only into the cc1plus it was built for.
readonly PLUGIN="${BUILD_DIR}/avr-flash-vtbl.so"
readonly CMAKE_PLUGIN="${BUILD_DIR}/avr-flash-vtbl-cmake.so"

# Every example in three variants: "ram" (vtables in RAM, the default of avr-gcc), "flash"
# (vtables in flash, with the plugin avr-flash-vtbl) and "checked" (vtables in RAM, misuse
# reported with UTL_CHECK_ACCESS=1). utl itself has no vtable since its groups lost their
# virtual functions; "flash" still moves the core's. clang-tidy reads the two whose defines
# differ.
VARIANTS=(ram flash checked)
TIDY_VARIANTS=(default checked)

variant_flags() {
  case "$1" in
    # extra_flags of arduino-cli, and VARIANT_FLAGS of CMakeLists.txt, reach every C++ file of
    # the build, the core included. A file compiled without the plugin would read a vtable in
    # flash from RAM. The plugin of the avr-gcc the build system takes: arduino-cli the core's,
    # CMake the one from apt.
    flash)
      if [[ "${BUILD_SYSTEM}" == cmake ]]; then
        echo "-fplugin=${CMAKE_PLUGIN}"
      else
        echo "-fplugin=${PLUGIN}"
      fi
      ;;
    checked) echo "-DUTL_CHECK_ACCESS=1" ;;
  esac
}

# Builds the plugins, against the avr-gcc of the core and the one from apt; again after the core
# is updated.
prepare() {
  select_target board
  local bin_dir
  bin_dir="$(avr_gcc_bin_dir)"
  if [[ -z "${bin_dir}" ]]; then
    err "arduino-cli knows no avr-gcc for ${FQBN}; install arduino:avr."
    return 1
  fi
  "${ROOT_DIR}/tools/build_plugin.sh" "${bin_dir}/avr-g++" "${PLUGIN}"
  "${ROOT_DIR}/tools/build_plugin.sh" avr-g++ "${CMAKE_PLUGIN}"
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
