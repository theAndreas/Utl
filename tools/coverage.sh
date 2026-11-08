#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs the tests on the host with --coverage and reports which lines of src/
# they reach: the lines no test reaches on the terminal, and all of it as HTML
# in build/coverage/html/. The variants of CMakeLists.txt count together, as
# each takes other branches of #if. gcovr comes from PyPI through uvx, at a
# fixed version, like the tools of lint.sh.

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

readonly GCOVR="gcovr==8.8"
readonly COVERAGE_DIR="${BUILD_DIR}/coverage"

# Removes the notes (.gcno) of a source that no longer exists, renamed or
# deleted: the build writes notes only for what it compiles and never
# removes old ones, and gcovr reads every one it finds, so the lines of a
# header that only such a file reached stood as missed. A note lies at
# CMakeFiles/<target>.dir/<source>.gcno, which gives its source.
remove_stale_notes() {
  local note
  local source
  while IFS= read -r -d '' note; do
    source="${note#*.dir/}"
    source="${ROOT_DIR}/${source%.gcno}"
    if [[ ! -f "${source}" ]]; then
      rm -f "${note}"
    fi
  done < <(find "${COVERAGE_DIR}" -name '*.gcno' -print0 2>/dev/null)
}

main() {
  cd "${ROOT_DIR}"
  # Counts from an earlier run would add to this one.
  find "${COVERAGE_DIR}" -name '*.gcda' -delete 2>/dev/null || true
  cmake --workflow --preset coverage
  remove_stale_notes

  mkdir -p "${COVERAGE_DIR}/html"
  # separate: a function under an #if of a variant lies on another line in
  # each, and is two functions, not one to merge.
  # --merge-lines: gcov counts a template per instance, and each lambda is a
  # type of its own, so a line reached by one instance stood as missed by
  # another; merged, a line counts as reached if any instance reaches it.
  # --exclude-throw-branches: on the host a call that may throw has a branch
  # to its exception, which no test takes; a sketch is built without
  # exceptions, so on the AVR there is no such branch to reach.
  uvx "${GCOVR}" --root "${ROOT_DIR}" --filter 'src/' \
    --merge-mode-functions=separate \
    --merge-lines \
    --exclude-throw-branches \
    --txt-metric line --txt "${COVERAGE_DIR}/coverage.txt" \
    --html-details "${COVERAGE_DIR}/html/index.html" \
    --print-summary "${COVERAGE_DIR}"
  echo
  # The files with lines missed, and which.
  grep -v ' 100%' "${COVERAGE_DIR}/coverage.txt" || true
}

main "$@"
