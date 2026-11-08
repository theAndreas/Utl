# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# The project's own targets, which the Makefile includes; the template wrote this file once, and
# `copier update` leaves it alone. A target with a `## ` comment is listed by `make`, and one
# added to PROJECT_CHECKS is part of `make check`:
#
#   PROJECT_CHECKS += sizes
#
#   sizes: ## The README's table of flash and RAM against the build
#   	tools/sizes.sh

## Utl's own

# Not in PROJECT_CHECKS: simavr runs the sketch far slower than the board, about 40 s a variant,
# so CI runs it apart, in .github/workflows/keypad.yml, for a change to what it uses. Without
# "flash" by default, which adds nothing here: the vtables of the core are not what the keys
# test.
KEYPAD_VARIANTS ?= ram checked

keypad: ## examples/Keypad in simavr with keys pressed, ~40 s a variant
	tools/keypad_check.sh $(KEYPAD_VARIANTS)
