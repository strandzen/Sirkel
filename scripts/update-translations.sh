#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# Regenerates the translation templates in po/ and merges them into every
# existing translation (po/<language>/*.po). Run from anywhere.
set -euo pipefail
cd "$(dirname "$0")/.."

# The same keywords KDE's own extraction uses for KI18n.
export XGETTEXT="xgettext --from-code=UTF-8 -C --kde --package-name=sirkel --msgid-bugs-address=https://github.com/strandzen/Sirkel/issues
    -ci18n -ki18n:1 -ki18nc:1c,2 -ki18np:1,2 -ki18ncp:1c,2,3 -ki18nd:2 -ki18ndc:2c,3 -ki18ndp:2,3 -ki18ndcp:2c,3,4
    -kki18n:1 -kki18nc:1c,2 -kki18np:1,2 -kki18ncp:1c,2,3
    -kkli18n:1 -kkli18nc:1c,2 -kkli18np:1,2 -kkli18ncp:1c,2,3
    -kI18N_NOOP:1 -kI18NC_NOOP:1c,2"
export podir=po
bash Messages.sh

for po in po/*/*.po; do
    [ -e "$po" ] || continue
    msgmerge --quiet --update --backup=none "$po" "po/$(basename "$po" .po).pot"
done
