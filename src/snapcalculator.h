#pragma once

#include <QRectF>

enum class SnapDirection {
    None,
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    // The pointer parked in the indicator's own center deadzone — maximizes
    // the window (padded, like every other direction) instead of snapping
    // to a half/corner.
    Maximize,
};

QString snapDirectionName(SnapDirection direction);

// Decides which direction the pointer has dragged towards, given its total
// displacement since the hyperkey was pressed. Horizontal and vertical
// components are each "armed" independently once they clear their own
// deadzone, so a diagonal drag can select a corner, and a pure vertical
// drag selects the top/bottom half.
SnapDirection snapDirectionForDelta(const QPointF &delta, qreal horizontalDeadzone, qreal verticalDeadzone);

// Maps a direction onto a concrete target geometry within the given usable
// screen area (the active window's screen only — windows never move across
// screens), inset by `paddingHorizontal` on the left/right edges and
// `paddingVertical` on the top/bottom edges. Returns an empty rect for
// SnapDirection::None.
//
// `sizeFraction` is the scroll-resize step (see SirkelEffect::pointerAxis):
// for Left/Right it scales width only (height stays full), for Top/Bottom
// it scales height only (width stays full), and for Maximize it also
// scales width only, centered horizontally (height stays full, same as the
// ordinary maximize). Corners ignore it - scroll resizing is edge/maximize
// only. Leave negative (the default) to use each direction's own normal
// size: 0.5 for the half-snap directions, 1.0 for Maximize.
QRectF snapTargetGeometry(SnapDirection direction, const QRectF &area, qreal paddingHorizontal = 0, qreal paddingVertical = 0, qreal sizeFraction = -1.0);
