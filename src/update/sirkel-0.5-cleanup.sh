#!/bin/sh
# kconf_update script (see sirkel.upd): drop settings removed in 0.5.

GROUP=Effect-sirkel

for key in LinkIndicatorColors OutlineConfirmFlash OutlineGradientFill; do
    if [ -n "$(kreadconfig6 --file kwinrc --group "$GROUP" --key "$key")" ]; then
        kwriteconfig6 --file kwinrc --group "$GROUP" --key "$key" --delete
    fi
done
