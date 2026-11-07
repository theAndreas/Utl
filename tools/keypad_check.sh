#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs examples/Keypad in simavr with keys pressed on its pins, and compares
# what it prints with Keypad.expected: the code entered after a wrong key
# opens the lock, and the next key closes it. The presses come from a VCD
# file, written here, which simavr reads as input signals.
#
# Not part of CI: simavr runs the sketch far slower than the board, about 40 s
# for the 0.3 s of presses, in each variant. .github/workflows/keypad.yml runs
# it for a pull request that touches what it checks, and by hand.
#
# Usage: tools/keypad_check.sh [variant...]  (default: every variant)

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

readonly EXAMPLE="Keypad"
readonly EXPECTED="${ROOT_DIR}/examples/${EXAMPLE}/${EXAMPLE}.expected"
readonly TIMEOUT_SECONDS=300
# The keys, in the order pressed: D, then the code CABA, then B, which closes
# the lock again. Each is down for 20 ms, 40 ms after the last, beyond the
# 30 ms the sketch takes for the bounce of a contact.
readonly PRESSES="DCABAB"
readonly PRESS_MS=20
readonly STEP_MS=40

# Writes the VCD file of the presses to stdout. simavr drives a pin by the
# name of its IRQ, iogB_<n> for PB<n>: A to D are PB0 to PB3, pins 8 to 11,
# pulled high and pressed low. simavr ends when the file does, so a last
# change lies after the last press.
write_presses() {
  local -r ms=1000000  # the timescale is 1 ns
  local time_ms=20
  local key
  local i

  # One signal per key, A to D on PB0 to PB3, named a to d in the changes.
  cat <<'EOF'
$timescale 1ns $end
$scope module keypad $end
$var wire 1 a iogB_0 $end
$var wire 1 b iogB_1 $end
$var wire 1 c iogB_2 $end
$var wire 1 d iogB_3 $end
$upscope $end
$enddefinitions $end
#0
1a
1b
1c
1d
EOF
  for ((i = 0; i < ${#PRESSES}; ++i)); do
    key="${PRESSES:i:1}"
    key="${key,}"  # its signal: A is a
    echo "#$((time_ms * ms))"
    echo "0${key}"
    echo "#$(((time_ms + PRESS_MS) * ms))"
    echo "1${key}"
    time_ms=$((time_ms + STEP_MS))
  done
  echo "#$(((time_ms + STEP_MS) * ms))"
  echo '1a'
}

main() {
  local variants=("$@")
  if ((${#variants[@]} == 0)); then
    variants=("${VARIANTS[@]}")
  fi
  local presses="${BUILD_DIR}/${EXAMPLE}.vcd"
  local failed=0
  local variant
  local output
  local actual
  local status

  mkdir -p "${BUILD_DIR}"
  write_presses > "${presses}"
  for variant in "${variants[@]}"; do
    if ! output="$(compile_example "${variant}" "${EXAMPLE}")"; then
      err "${EXAMPLE} (${variant}) does not compile:"
      err "${output}"
      failed=1
      continue
    fi
    status=0
    # simavr writes the UART to stderr, a line in color with CR LF shown as
    # "..", and its own messages to stdout.
    actual="$(timeout "${TIMEOUT_SECONDS}" simavr -m atmega328p -f 16000000 \
      -i "${presses}" "$(elf_of "${variant}" "${EXAMPLE}")" 2>&1 >/dev/null \
      | sed -e 's/\x1b\[[0-9;]*m//g' -e 's/\.\.$//' | grep -v '^$')" \
      || status=$?
    if ((status != 0)); then
      err "${EXAMPLE} (${variant}): simavr ended with ${status}."
      failed=1
    fi
    if diff -u --label expected --label "${variant}" "${EXPECTED}" \
        <(echo "${actual}"); then
      echo "${EXAMPLE} (${variant}): as expected"
    else
      failed=1
    fi
  done
  exit "${failed}"
}

main "$@"
