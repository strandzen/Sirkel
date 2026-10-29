// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QColor>

// Resolves the configured colors from SirkelConfig. Shared by the effect
// (inside kwin_wayland) and the KCM (inside System Settings), which each have
// their own SirkelConfig instance.
enum class ColorTarget {
    Outline,
    Pointer,
    Ring
};

// Re-reads kdeglobals if it changed, so a changed Plasma accent color is
// picked up. Call once before resolving a set of colors (the effect does it
// per session), not per color. The caller keeps kdeglobals open
// (KSharedConfig::openConfig("kdeglobals")) so it isn't parsed per color.
void reloadColorSources();

// The color for `target` per its *ColorSource setting: "manual" (the stored
// color) or "theme" (the Plasma accent, from kdeglobals rather than the
// process palette, which isn't guaranteed to follow accent changes live).
// The ring gets a dark, muted version of the accent.
QColor resolveColor(ColorTarget target);
