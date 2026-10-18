#pragma once

#include <QColor>

// Resolves the configured colors from SirkelConfig. Shared by the effect
// (inside kwin_wayland) and the KCM (inside System Settings), which each have
// their own SirkelConfig instance.
enum class ColorTarget { Outline, Pointer, Ring };

// The color for `target` per its *ColorSource setting: "manual" (the stored
// color), "theme" (the live Plasma accent) or "wallpaper" (WallpaperColorPath,
// falling back to the accent if unreadable). Ignores LinkIndicatorColors.
QColor resolveColor(ColorTarget target);

// Same, but honours LinkIndicatorColors: when linked, pointer and ring use
// the outline's color.
QColor resolveLinkedColor(ColorTarget target);

// The Plasma accent color, read fresh from kdeglobals rather than from the
// process palette, which isn't guaranteed to follow accent changes live.
QColor accentColor();
