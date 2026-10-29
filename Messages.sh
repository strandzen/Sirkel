#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# Extracts translatable strings into one template per translation domain.
# KDE's tooling calls this with $XGETTEXT and $podir set; to run it by hand,
# use scripts/update-translations.sh. Files are sorted so the templates come
# out the same on every machine (find's order depends on the filesystem).

# The effect, the code it shares with the settings, and sirkel-rebuild.
$XGETTEXT $({ find src -maxdepth 1 \( -name '*.cpp' -o -name '*.h' \); find src/rebuild \( -name '*.cpp' -o -name '*.h' \); } | LC_ALL=C sort) -o $podir/sirkel.pot
# The settings window: its C++ and its QML pages.
$XGETTEXT $(find src/kcm \( -name '*.cpp' -o -name '*.h' -o -name '*.qml' \) | LC_ALL=C sort) -o $podir/kwin_effect_sirkel_config.pot
