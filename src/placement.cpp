#include "placement.h"

#include <KLazyLocalizedString>

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
// Scroll-resize cycles. Maximize deliberately has no quarter step.
const QList<qreal> kEdgeSteps = {1.0 / 4.0, 1.0 / 3.0, 1.0 / 2.0, 2.0 / 3.0};
const QList<qreal> kMaximizeSteps = {1.0 / 3.0, 1.0 / 2.0, 1.0};

struct PlacementRow {
    Placement placement;
    KLazyLocalizedString label;
};

// Corners resize width only; their height always stays at half.
const std::array<PlacementRow, 9> kPlacements{{
    {{SnapDirection::Left, "left", Anchor::Start, Anchor::Start, 0.5, 1.0, ResizeAxis::Width, &kEdgeSteps, 2, 6}, kli18n("Left")},
    {{SnapDirection::Right, "right", Anchor::End, Anchor::Start, 0.5, 1.0, ResizeAxis::Width, &kEdgeSteps, 2, 2}, kli18n("Right")},
    {{SnapDirection::Top, "top", Anchor::Start, Anchor::Start, 1.0, 0.5, ResizeAxis::Height, &kEdgeSteps, 2, 0}, kli18n("Top")},
    {{SnapDirection::Bottom, "bottom", Anchor::Start, Anchor::End, 1.0, 0.5, ResizeAxis::Height, &kEdgeSteps, 2, 4}, kli18n("Bottom")},
    {{SnapDirection::TopLeft, "topLeft", Anchor::Start, Anchor::Start, 0.5, 0.5, ResizeAxis::Width, &kEdgeSteps, 2, 7}, kli18n("Top\nLeft")},
    {{SnapDirection::TopRight, "topRight", Anchor::End, Anchor::Start, 0.5, 0.5, ResizeAxis::Width, &kEdgeSteps, 2, 1}, kli18n("Top\nRight")},
    {{SnapDirection::BottomLeft, "bottomLeft", Anchor::Start, Anchor::End, 0.5, 0.5, ResizeAxis::Width, &kEdgeSteps, 2, 5}, kli18n("Bottom\nLeft")},
    {{SnapDirection::BottomRight, "bottomRight", Anchor::End, Anchor::End, 0.5, 0.5, ResizeAxis::Width, &kEdgeSteps, 2, 3}, kli18n("Bottom\nRight")},
    // Shrinks in from both sides, centered, height stays full.
    {{SnapDirection::Maximize, "maximize", Anchor::Center, Anchor::Start, 1.0, 1.0, ResizeAxis::Width, &kMaximizeSteps, 2, -1}, kli18n("Maximize")},
}};

const PlacementRow *rowFor(SnapDirection direction)
{
    for (const PlacementRow &row : kPlacements) {
        if (row.placement.direction == direction) {
            return &row;
        }
    }
    return nullptr;
}

qreal anchoredStart(Anchor anchor, qreal areaStart, qreal areaLength, qreal length)
{
    switch (anchor) {
    case Anchor::Start:
        return areaStart;
    case Anchor::Center:
        return areaStart + (areaLength - length) / 2.0;
    case Anchor::End:
        return areaStart + areaLength - length;
    }
    return areaStart;
}
}

const Placement *placementFor(SnapDirection direction)
{
    const PlacementRow *row = rowFor(direction);
    return row ? &row->placement : nullptr;
}

QString placementName(SnapDirection direction)
{
    const Placement *p = placementFor(direction);
    return p ? QString::fromLatin1(p->name) : QStringLiteral("none");
}

SnapDirection directionFromName(const QString &name)
{
    for (const PlacementRow &row : kPlacements) {
        if (name == QLatin1String(row.placement.name)) {
            return row.placement.direction;
        }
    }
    return SnapDirection::None;
}

QString placementLabel(SnapDirection direction)
{
    const PlacementRow *row = rowFor(direction);
    return row ? row->label.toString() : QString();
}

int normalStep(SnapDirection direction)
{
    const Placement *p = placementFor(direction);
    return p ? p->normalStep : 0;
}

int cycleStep(SnapDirection direction, int step, int delta)
{
    const Placement *p = placementFor(direction);
    if (!p || p->steps->isEmpty()) {
        return step;
    }
    const int count = p->steps->size();
    return ((step + delta) % count + count) % count;
}

QRectF placementGeometry(SnapDirection direction, const QRectF &area, qreal paddingHorizontal, qreal paddingVertical, int step)
{
    const Placement *p = placementFor(direction);
    if (!p) {
        return QRectF();
    }
    const int count = p->steps->size();
    const int idx = step < 0 ? p->normalStep : ((step % count) + count) % count;
    const qreal fraction = p->steps->at(idx);

    const qreal w = area.width() * (p->resizeAxis == ResizeAxis::Width ? fraction : p->widthFraction);
    const qreal h = area.height() * (p->resizeAxis == ResizeAxis::Height ? fraction : p->heightFraction);
    const QRectF rect(anchoredStart(p->horizontal, area.x(), area.width(), w), anchoredStart(p->vertical, area.y(), area.height(), h), w, h);

    // An edge that doesn't touch the area's border faces a neighbour.
    constexpr qreal eps = 0.5;
    const bool sharedLeft = rect.left() > area.left() + eps;
    const bool sharedRight = rect.right() < area.right() - eps;
    const bool sharedTop = rect.top() > area.top() + eps;
    const bool sharedBottom = rect.bottom() < area.bottom() - eps;
    return rect.adjusted(sharedLeft ? paddingHorizontal / 2.0 : paddingHorizontal,
                         sharedTop ? paddingVertical / 2.0 : paddingVertical,
                         -(sharedRight ? paddingHorizontal / 2.0 : paddingHorizontal),
                         -(sharedBottom ? paddingVertical / 2.0 : paddingVertical));
}

QPoint placementEdges(SnapDirection direction)
{
    const Placement *p = placementFor(direction);
    if (!p) {
        return {};
    }
    // An axis only pulls against an edge if the placement is constrained on
    // it (not full width/height) and not centered.
    const auto edge = [](Anchor anchor, bool constrained) {
        if (!constrained || anchor == Anchor::Center) {
            return 0;
        }
        return anchor == Anchor::Start ? -1 : 1;
    };
    return QPoint(edge(p->horizontal, p->resizeAxis == ResizeAxis::Width || p->widthFraction < 1.0),
                  edge(p->vertical, p->resizeAxis == ResizeAxis::Height || p->heightFraction < 1.0));
}

QRectF centeredFixedSize(const QRectF &target, const QSizeF &size, const QRectF &area)
{
    QRectF rect(QPointF(), size);
    rect.moveCenter(target.center());
    // Shift back inside the area; a window larger than the area keeps its
    // top-left corner on screen.
    rect.moveLeft(std::max(area.left(), std::min(rect.left(), area.right() - rect.width())));
    rect.moveTop(std::max(area.top(), std::min(rect.top(), area.bottom() - rect.height())));
    return rect;
}

SnapDirection directionForDelta(const QPointF &delta, qreal horizontalDeadzone, qreal verticalDeadzone)
{
    const bool hasHorizontal = std::abs(delta.x()) >= horizontalDeadzone;
    const bool hasVertical = std::abs(delta.y()) >= verticalDeadzone;
    const bool right = delta.x() > 0;
    const bool down = delta.y() > 0;

    if (hasHorizontal && hasVertical) {
        if (right) {
            return down ? SnapDirection::BottomRight : SnapDirection::TopRight;
        }
        return down ? SnapDirection::BottomLeft : SnapDirection::TopLeft;
    }
    if (hasHorizontal) {
        return right ? SnapDirection::Right : SnapDirection::Left;
    }
    if (hasVertical) {
        return down ? SnapDirection::Bottom : SnapDirection::Top;
    }
    return SnapDirection::None;
}

SnapDirection directionForAngle(const QPointF &delta, qreal centerRadius, qreal cardinalBias)
{
    if (std::hypot(delta.x(), delta.y()) < centerRadius) {
        return SnapDirection::None;
    }
    // Clockwise from Right, in screen coordinates (y grows downwards).
    static constexpr std::array<SnapDirection, 8> kSlices{
        SnapDirection::Right,
        SnapDirection::BottomRight,
        SnapDirection::Bottom,
        SnapDirection::BottomLeft,
        SnapDirection::Left,
        SnapDirection::TopLeft,
        SnapDirection::Top,
        SnapDirection::TopRight,
    };
    qreal degrees = std::atan2(delta.y(), delta.x()) * 180.0 / M_PI;
    if (degrees < 0) {
        degrees += 360.0;
    }
    const qreal shifted = std::fmod(degrees + 22.5, 360.0);
    int slice = int(shifted / 45.0) % 8;
    // Odd slices are corners; hand their outer edges to the neighbouring
    // edge slices.
    if (slice % 2 == 1) {
        const qreal position = std::fmod(shifted, 45.0) / 45.0;
        if (position < cardinalBias / 2.0) {
            slice = (slice + 7) % 8;
        } else if (position > 1.0 - cardinalBias / 2.0) {
            slice = (slice + 1) % 8;
        }
    }
    return kSlices[slice];
}
