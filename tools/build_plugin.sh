#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds the GCC plugin avr-flash-vtbl against the plugin headers of an avr-g++. A plugin only
# loads into the cc1plus it was built for, so each avr-gcc the targets use gets its own, and the
# one of arduino-cli is built again after the AVR core is updated.
#
# Usage:
#   tools/build_plugin.sh AVR-G++ PLUGIN

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

main() {
  if [[ $# -ne 2 ]]; then
    err "Usage: $0 AVR-G++ PLUGIN"
    exit 2
  fi
  local compiler="$1"
  local plugin="$2"

  local plugin_dir
  plugin_dir="$("${compiler}" -print-file-name=plugin)"
  if [[ ! -d "${plugin_dir}/include" ]]; then
    err "${plugin_dir}/include is missing: ${compiler} has no plugin headers."
    exit 1
  fi

  mkdir -p "$(dirname "${plugin}")"
  g++ -shared -fPIC -fno-rtti -O2 \
    -I"${plugin_dir}/include" \
    "${ROOT_DIR}/tools/avr-flash-vtbl/avr-flash-vtbl.c" \
    -o "${plugin}"
  echo "Built ${plugin}"
}

main "$@"
