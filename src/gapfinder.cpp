// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gapfinder.h"

#include <algorithm>

namespace
{
// Below this, edges count as the same (sub-pixel noise, rounding).
constexpr qreal kEpsilon = 0.5;

bool contains(const QRectF &outer, const QRectF &inner)
{
    return inner.left() >= outer.left() - kEpsilon && inner.right() <= outer.right() + kEpsilon && inner.top() >= outer.top() - kEpsilon
        && inner.bottom() <= outer.bottom() + kEpsilon;
}

// Sorted, without near-duplicates.
QList<qreal> uniqueSorted(QList<qreal> values)
{
    std::sort(values.begin(), values.end());
    QList<qreal> out;
    for (qreal v : values) {
        if (out.isEmpty() || v - out.last() > kEpsilon) {
            out.append(v);
        }
    }
    return out;
}
}

QList<QRectF> freeSpaces(const QRectF &area, const QList<QRectF> &obstacles)
{
    QList<QRectF> clipped;
    for (const QRectF &obstacle : obstacles) {
        const QRectF part = obstacle.intersected(area);
        if (part.width() > kEpsilon && part.height() > kEpsilon) {
            clipped.append(part);
        }
    }

    // Every maximal free rectangle has its left and right edges on the
    // area's border or an obstacle's edge. For each such pair, the free
    // rectangles are the vertical runs not blocked by an obstacle in that
    // column; those that aren't inside another one are the maximal ones.
    QList<qreal> xs{area.left(), area.right()};
    for (const QRectF &o : std::as_const(clipped)) {
        xs << o.left() << o.right();
    }
    xs = uniqueSorted(xs);

    QList<QRectF> candidates;
    for (int i = 0; i < xs.size(); ++i) {
        for (int j = i + 1; j < xs.size(); ++j) {
            const qreal left = xs[i];
            const qreal right = xs[j];
            QList<std::pair<qreal, qreal>> blocked;
            for (const QRectF &o : std::as_const(clipped)) {
                if (o.left() < right - kEpsilon && left < o.right() - kEpsilon) {
                    blocked.append({o.top(), o.bottom()});
                }
            }
            std::sort(blocked.begin(), blocked.end());
            qreal y = area.top();
            for (const auto &[top, bottom] : std::as_const(blocked)) {
                if (top - y > kEpsilon) {
                    candidates.append(QRectF(QPointF(left, y), QPointF(right, top)));
                }
                y = std::max(y, bottom);
            }
            if (area.bottom() - y > kEpsilon) {
                candidates.append(QRectF(QPointF(left, y), QPointF(right, area.bottom())));
            }
        }
    }

    QList<QRectF> spaces;
    for (int i = 0; i < candidates.size(); ++i) {
        bool inside = false;
        for (int j = 0; j < candidates.size() && !inside; ++j) {
            // Inside a bigger one (equal ones: keep the first).
            inside = j != i && contains(candidates[j], candidates[i]) && (!contains(candidates[i], candidates[j]) || j < i);
        }
        if (!inside) {
            spaces.append(candidates[i]);
        }
    }
    return spaces;
}

std::optional<QRectF> pickFreeSpace(const QList<QRectF> &spaces, const QRectF &area, SnapDirection direction, const QSizeF &minSize)
{
    const QPointF middle = area.center();
    const bool wantLeft = direction == SnapDirection::Left || direction == SnapDirection::TopLeft || direction == SnapDirection::BottomLeft;
    const bool wantRight = direction == SnapDirection::Right || direction == SnapDirection::TopRight || direction == SnapDirection::BottomRight;
    const bool wantTop = direction == SnapDirection::Top || direction == SnapDirection::TopLeft || direction == SnapDirection::TopRight;
    const bool wantBottom = direction == SnapDirection::Bottom || direction == SnapDirection::BottomLeft || direction == SnapDirection::BottomRight;

    // How far a space lies toward the wanted side(s), for ties.
    const auto towardSide = [&](const QRectF &r) {
        qreal d = 0;
        if (wantLeft) {
            d -= r.center().x();
        }
        if (wantRight) {
            d += r.center().x();
        }
        if (wantTop) {
            d -= r.center().y();
        }
        if (wantBottom) {
            d += r.center().y();
        }
        return d;
    };

    std::optional<QRectF> best;
    for (const QRectF &space : spaces) {
        if (space.width() < minSize.width() - kEpsilon || space.height() < minSize.height() - kEpsilon) {
            continue;
        }
        // At least minSize of it on that side, not just a sliver past the
        // middle (a padded gap ends a few pixels beyond it).
        if ((wantLeft && middle.x() - space.left() < minSize.width() - kEpsilon) || (wantRight && space.right() - middle.x() < minSize.width() - kEpsilon)
            || (wantTop && middle.y() - space.top() < minSize.height() - kEpsilon)
            || (wantBottom && space.bottom() - middle.y() < minSize.height() - kEpsilon)) {
            continue;
        }
        const qreal area1 = space.width() * space.height();
        if (!best) {
            best = space;
            continue;
        }
        const qreal area2 = best->width() * best->height();
        if (area1 > area2 + kEpsilon || (area1 > area2 - kEpsilon && towardSide(space) > towardSide(*best))) {
            best = space;
        }
    }
    return best;
}

QRectF paddedFreeSpace(const QRectF &space, const QRectF &area, qreal paddingHorizontal, qreal paddingVertical)
{
    const auto pad = [](bool atBorder, qreal padding) {
        return atBorder ? padding : padding / 2.0;
    };
    return space.adjusted(pad(space.left() <= area.left() + kEpsilon, paddingHorizontal),
                          pad(space.top() <= area.top() + kEpsilon, paddingVertical),
                          -pad(space.right() >= area.right() - kEpsilon, paddingHorizontal),
                          -pad(space.bottom() >= area.bottom() - kEpsilon, paddingVertical));
}
