#!/bin/sh
# kconf_update script (see sirkel.upd): clean up Sirkel settings renamed or
# removed during 0.1/0.2 development. Uses kreadconfig6/kwriteconfig6 so
# kwinrc's format is handled by KConfig itself. Only touches keys that exist,
# and never overwrites a value the user already has under the new name.

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

# Moves OLD's value to NEW unless NEW is already set, then drops OLD.
rename_key() {
    value=$(read_key "$1")
    if [ -n "$value" ]; then
        if [ -z "$(read_key "$2")" ]; then
            write_key "$2" "$value"
        fi
        delete_key "$1"
    fi
}

rename_key IndicatorColor IndicatorPointerColor
rename_key IndicatorColorSource IndicatorPointerColorSource
rename_key LinkIndicatorOutlineColor LinkIndicatorColors
rename_key IndicatorOpacity IndicatorRingFillOpacity

# OvershootEasing (bool) became Easing ("overshoot", "smooth", ...).
overshoot=$(read_key OvershootEasing)
if [ -n "$overshoot" ]; then
    if [ -z "$(read_key Easing)" ]; then
        if [ "$overshoot" = "true" ]; then
            write_key Easing overshoot
        else
            write_key Easing smooth
        fi
    fi
    delete_key OvershootEasing
fi

# The "doubleclick" trigger mode was removed.
if [ "$(read_key TriggerMode)" = "doubleclick" ]; then
    write_key TriggerMode instant
fi

# Settings that no longer exist.
for key in EntranceAnimation IndicatorBlur IndicatorDirectionIntensity \
           IndicatorMaximizeBreathe IndicatorRingBlur OutlineGlow \
           ScrollResizeLargePercent ScrollResizeSmallPercent; do
    if [ -n "$(read_key "$key")" ]; then
        delete_key "$key"
    fi
done

exit 0
