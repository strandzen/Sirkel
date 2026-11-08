// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "placement.h"

#include <QList>
#include <QRectF>
#include <QSizeF>

#include <optional>

// Fill the gap: the free space between the other windows on a screen, as
// the largest free rectangles (no grid; like Loop's "fill available
// space", with the direction choosing the side). Pure logic, no KWin, so
// it can be unit tested.

// The maximal free rectangles of `area` with `obstacles` (other windows'
// frames) cut out: free, and not inside another free rectangle. Obstacles
// are clipped to the area.
QList<QRectF> freeSpaces(const QRectF &area, const QList<QRectF> &obstacles);

// The free space for `direction`: among `spaces` at least `minSize` big,
// those reaching at least `minSize` into that half of the area (corners:
// both halves;
// Maximize: any), the largest; a tie goes to the one further that way.
// A space across the middle counts for every side, so with nothing else
// on screen any direction fills it. Nothing if no space qualifies.
std::optional<QRectF> pickFreeSpace(const QList<QRectF> &spaces, const QRectF &area, SnapDirection direction, const QSizeF &minSize);

// `space` with Sirkel's padding, as placements get it: the full padding on
// edges against the area's border, half on edges against a window (whose
// own placement has the other half), so it lines up with snapped
// neighbours.
QRectF paddedFreeSpace(const QRectF &space, const QRectF &area, qreal paddingHorizontal, qreal paddingVertical);
