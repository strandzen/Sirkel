#pragma once

#include <QColor>

// Resolves the configured colors from SirkelConfig. Shared by the effect
// (inside kwin_wayland) and the KCM (inside System Settings), which each have
// their own SirkelConfig instance.
enum class ColorTarget { Outline, Pointer, Ring };

// Re-reads kdeglobals, so a changed Plasma accent color is picked up. Call
// once before resolving a set of colors (the effect does it per session),
// not per color.
void reloadColorSources();

// The color for `target` per its *ColorSource setting: "manual" (the stored
// color), "theme" (the Plasma accent, from kdeglobals rather than the
// process palette, which isn't guaranteed to follow accent changes live) or
// "wallpaper" (WallpaperColorPath, falling back to the accent if
// unreadable). Honours LinkIndicatorColors: when linked, pointer and ring
// use the outline's color.
QColor resolveLinkedColor(ColorTarget target);
