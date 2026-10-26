#!/bin/sh
# SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# kconf_update script (see sirkel.upd): settings simplified after 0.5.1.
# Only touches keys that exist, and never overwrites a value already set
# under the new name.

GROUP=Effect-sirkel

read_key() {
    kreadconfig6 --file kwinrc --group "$GROUP" --key "$1"
}

write_key() {
    kwriteconfig6 --file kwinrc --group "$GROUP" --key "$1" "$2"
}

delete_key() {
    kwriteconfig6 --file kwinrc --group "$GROUP" --key "$1" --delete
}

# The horizontal and vertical deadzones became one Deadzone. Keeps the
# horizontal value (the vertical one if only that was set).
horizontal=$(read_key HorizontalDeadzone)
vertical=$(read_key VerticalDeadzone)
if [ -z "$(read_key Deadzone)" ]; then
    if [ -n "$horizontal" ]; then
        write_key Deadzone "$horizontal"
    elif [ -n "$vertical" ]; then
        write_key Deadzone "$vertical"
    fi
fi
delete_key HorizontalDeadzone
delete_key VerticalDeadzone

# The wallpaper color source was removed: such colors follow the theme.
for key in OutlineColorSource IndicatorPointerColorSource IndicatorRingColorSource; do
    if [ "$(read_key "$key")" = "wallpaper" ]; then
        write_key "$key" theme
    fi
done

# Settings that no longer exist.
for key in ShowIndicatorText IndicatorPointerLength TriggerMode TriggerDelay \
           Shadow TwoTone WallpaperColorPath; do
    if [ -n "$(read_key "$key")" ]; then
        delete_key "$key"
    fi
done
