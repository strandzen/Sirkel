#pragma once

#include <QPointF>
#include <QRectF>
#include <QString>

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
    // The pointer parked in the indicator's center deadzone. Full width and
    // height at its normal size, padded like every other placement.
    Maximize,
};

// Where a placement sits along one axis of the screen area.
enum class Anchor { Start, Center, End };

// Which dimension scroll-resize changes for a placement.
enum class ResizeAxis { Width, Height };

// Everything that defines one placement. Adding a new placement means adding
// an enum value and one row to the table in placement.cpp - geometry,
// padding, scroll-resize, the indicator label and ring position all derive
// from it.
struct Placement {
    SnapDirection direction;
    // Stable id shared with QML (Outline/Indicator) and debug output.
    const char *name;
    Anchor horizontal;
    Anchor vertical;
    // Size at the normal step, as a fraction of the usable area.
    qreal widthFraction;
    qreal heightFraction;
    ResizeAxis resizeAxis;
    // Scroll-resize cycle along resizeAxis, ascending, and which entry is the
    // normal size a fresh pick starts at.
    const QList<qreal> *steps;
    int normalStep;
    // Position on the indicator ring, clockwise from top (0..7), or -1 for a
    // placement with no ring position of its own (Maximize).
    int ringIndex;
};

// nullptr for SnapDirection::None.
const Placement *placementFor(SnapDirection direction);

QString placementName(SnapDirection direction);
// Translated, user-facing. Corner labels are split over two lines.
QString placementLabel(SnapDirection direction);

int normalStep(SnapDirection direction);
// Wraps around in both directions - scroll-resize cycles rather than clamps.
int cycleStep(SnapDirection direction, int step, int delta);

// The target geometry for `direction` within `area` (the usable area of the
// target screen), at scroll-resize `step` (-1 = the normal size). Edges
// against the area's own border get the full padding; edges facing into the
// area (shared with a neighbouring snapped window) get half, so two
// neighbours end up with the same gap between them as against the screen
// edge. Returns an empty rect for SnapDirection::None.
QRectF placementGeometry(SnapDirection direction, const QRectF &area, qreal paddingHorizontal, qreal paddingVertical, int step = -1);

// Where a window of fixed `size` (one that can't be resized) goes for a
// placement: centered in `target`, then kept inside `area` where possible.
QRectF centeredFixedSize(const QRectF &target, const QSizeF &size, const QRectF &area);

// Which placement a pointer displacement from the indicator's center selects.
// Each axis arms independently once past its own deadzone, so a diagonal drag
// selects a corner. Returns None inside both deadzones.
SnapDirection directionForDelta(const QPointF &delta, qreal horizontalDeadzone, qreal verticalDeadzone);

// Angle-based alternative to directionForDelta(): None within `centerRadius`
// of the center, otherwise one of eight 45 degree slices around it. The four
// edge slices are widened by `cardinalBias` (a fraction of a slice taken
// from the neighbouring corner slices) so a slightly sloppy horizontal or
// vertical move still reads as an edge.
SnapDirection directionForAngle(const QPointF &delta, qreal centerRadius, qreal cardinalBias = 0.2);
