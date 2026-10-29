// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "placement.h"

#include <QTest>

#include <cmath>

class PlacementTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void geometry_data();
    void geometry();
    void noneIsEmpty();
    void cycleWraps();
    void ringIndicesAreUnique();
    void fixedSizeCentered();
    void neighboursShareEdges();
    void namesRoundTrip();
    void directionForAngle_data();
    void directionForAngle();
    void directionForDelta_data();
    void directionForDelta();
};

// Expected values: full padding (10 horizontal, 20 vertical) against the
// area's border, half where an edge faces a neighbour.
void PlacementTest::geometry_data()
{
    QTest::addColumn<SnapDirection>("direction");
    QTest::addColumn<int>("step");
    QTest::addColumn<QRectF>("area");
    QTest::addColumn<QRectF>("expected");

    const QRectF area(0, 0, 1000, 800);
    QTest::newRow("left half") << SnapDirection::Left << -1 << area << QRectF(10, 20, 485, 760);
    QTest::newRow("right third") << SnapDirection::Right << 1 << area << QRectF(1000 - 1000.0 / 3 + 5, 20, 1000.0 / 3 - 15, 760);
    QTest::newRow("left quarter") << SnapDirection::Left << 0 << area << QRectF(10, 20, 235, 760);
    QTest::newRow("top half") << SnapDirection::Top << -1 << area << QRectF(10, 20, 980, 370);
    QTest::newRow("bottom two thirds") << SnapDirection::Bottom << 3 << area << QRectF(10, 800 - 1600.0 / 3 + 10, 980, 1600.0 / 3 - 30);
    QTest::newRow("bottom right") << SnapDirection::BottomRight << -1 << area << QRectF(505, 410, 485, 370);
    QTest::newRow("top left two thirds") << SnapDirection::TopLeft << 3 << area << QRectF(10, 20, 2000.0 / 3 - 15, 370);
    QTest::newRow("maximize") << SnapDirection::Maximize << -1 << area << QRectF(10, 20, 980, 760);
    QTest::newRow("maximize half") << SnapDirection::Maximize << 1 << area << QRectF(255, 20, 490, 760);
    QTest::newRow("offset area") << SnapDirection::Left << -1 << QRectF(100, 50, 1000, 800) << QRectF(110, 70, 485, 760);
}

void PlacementTest::geometry()
{
    QFETCH(SnapDirection, direction);
    QFETCH(int, step);
    QFETCH(QRectF, area);
    QFETCH(QRectF, expected);

    const QRectF actual = placementGeometry(direction, area, 10, 20, step);
    QVERIFY2(qAbs(actual.x() - expected.x()) < 0.01 && qAbs(actual.y() - expected.y()) < 0.01 && qAbs(actual.width() - expected.width()) < 0.01
                 && qAbs(actual.height() - expected.height()) < 0.01,
             qPrintable(QStringLiteral("got %1,%2 %3x%4").arg(actual.x()).arg(actual.y()).arg(actual.width()).arg(actual.height())));
}

void PlacementTest::noneIsEmpty()
{
    QVERIFY(placementGeometry(SnapDirection::None, QRectF(0, 0, 100, 100), 0, 0).isNull());
    QCOMPARE(placementName(SnapDirection::None), QStringLiteral("none"));
}

void PlacementTest::cycleWraps()
{
    QCOMPARE(normalStep(SnapDirection::Left), 2);
    QCOMPARE(cycleStep(SnapDirection::Left, 3, +1), 0);
    QCOMPARE(cycleStep(SnapDirection::Left, 0, -1), 3);
    QCOMPARE(cycleStep(SnapDirection::Maximize, 0, -1), 2);
}

void PlacementTest::ringIndicesAreUnique()
{
    QSet<int> seen;
    for (auto d : {SnapDirection::Left,
                   SnapDirection::Right,
                   SnapDirection::Top,
                   SnapDirection::Bottom,
                   SnapDirection::TopLeft,
                   SnapDirection::TopRight,
                   SnapDirection::BottomLeft,
                   SnapDirection::BottomRight}) {
        const int index = placementFor(d)->ringIndex;
        QVERIFY(index >= 0 && index < 8);
        QVERIFY(!seen.contains(index));
        seen.insert(index);
    }
    QCOMPARE(placementFor(SnapDirection::Maximize)->ringIndex, -1);
}

void PlacementTest::namesRoundTrip()
{
    for (auto d : {SnapDirection::Left,
                   SnapDirection::Right,
                   SnapDirection::Top,
                   SnapDirection::Bottom,
                   SnapDirection::TopLeft,
                   SnapDirection::TopRight,
                   SnapDirection::BottomLeft,
                   SnapDirection::BottomRight,
                   SnapDirection::Maximize}) {
        QCOMPARE(directionFromName(placementName(d)), d);
    }
    QCOMPARE(directionFromName(QStringLiteral("bogus")), SnapDirection::None);
}

void PlacementTest::fixedSizeCentered()
{
    const QRectF area(0, 0, 1000, 800);
    // Centered in the left half.
    QCOMPARE(centeredFixedSize(QRectF(0, 0, 500, 800), QSizeF(200, 100), area), QRectF(150, 350, 200, 100));
    // Wider than the target: centered on it, but pulled back on screen.
    QCOMPARE(centeredFixedSize(QRectF(0, 0, 250, 800), QSizeF(400, 100), area), QRectF(0, 350, 400, 100));
    // Larger than the whole area: top-left stays on screen.
    QCOMPARE(centeredFixedSize(QRectF(0, 0, 500, 800), QSizeF(1200, 900), area).topLeft(), QPointF(0, 0));
}

// Thirds of 3440 px aren't whole pixels. Rounded per edge, three windows
// side by side (left third, centered third, right third) meet exactly: no
// 1 px gap or overlap, and the gaps between them are the padding.
void PlacementTest::neighboursShareEdges()
{
    const QRectF area(0, 0, 3440, 1400);
    for (const qreal padding : {0.0, 10.0}) {
        const QRectF left = alignedToPixels(placementGeometry(SnapDirection::Left, area, padding, padding, 1));
        const QRectF middle = alignedToPixels(placementGeometry(SnapDirection::Maximize, area, padding, padding, 0));
        const QRectF right = alignedToPixels(placementGeometry(SnapDirection::Right, area, padding, padding, 1));
        for (const QRectF &rect : {left, middle, right}) {
            QCOMPARE(rect.left(), std::floor(rect.left()));
            QCOMPARE(rect.right(), std::floor(rect.right()));
        }
        QCOMPARE(middle.left() - left.right(), padding);
        QCOMPARE(right.left() - middle.right(), padding);
        QCOMPARE(right.right(), area.right() - padding);
    }
    // A whole-pixel size keeps its size wherever it sits, also on a screen
    // left of the origin (halves round the same way at both edges).
    QCOMPARE(alignedToPixels(QRectF(-100.5, 0.5, 800, 600)).size(), QSizeF(800, 600));
}

void PlacementTest::directionForAngle_data()
{
    QTest::addColumn<QPointF>("delta");
    QTest::addColumn<SnapDirection>("expected");

    QTest::newRow("center") << QPointF(20, 20) << SnapDirection::None;
    QTest::newRow("right") << QPointF(60, 0) << SnapDirection::Right;
    QTest::newRow("left") << QPointF(-60, 5) << SnapDirection::Left;
    QTest::newRow("top") << QPointF(0, -60) << SnapDirection::Top;
    QTest::newRow("bottom") << QPointF(3, 60) << SnapDirection::Bottom;
    // A short diagonal already reaches a corner: no per-axis minimum.
    QTest::newRow("top right short") << QPointF(32, -32) << SnapDirection::TopRight;
    QTest::newRow("bottom left") << QPointF(-50, 50) << SnapDirection::BottomLeft;
    // 60 right, 35 up is about 30 degrees: a corner. (60/30, 26.6 degrees,
    // would be Right: within the edge bias.)
    QTest::newRow("shallow diagonal") << QPointF(60, -35) << SnapDirection::TopRight;
    // 24 degrees: inside the corner slice's outer 4.5 degrees, so the edge
    // bias keeps it Right.
    QTest::newRow("cardinal bias") << QPointF(100, -44.5) << SnapDirection::Right;
}

void PlacementTest::directionForAngle()
{
    QFETCH(QPointF, delta);
    QFETCH(SnapDirection, expected);
    QCOMPARE(::directionForAngle(delta, 40), expected);
}

void PlacementTest::directionForDelta_data()
{
    QTest::addColumn<QPointF>("delta");
    QTest::addColumn<SnapDirection>("expected");

    QTest::newRow("center") << QPointF(10, -10) << SnapDirection::None;
    QTest::newRow("left") << QPointF(-60, 5) << SnapDirection::Left;
    QTest::newRow("right") << QPointF(60, 0) << SnapDirection::Right;
    QTest::newRow("top") << QPointF(0, -60) << SnapDirection::Top;
    QTest::newRow("bottom") << QPointF(5, 60) << SnapDirection::Bottom;
    QTest::newRow("top left") << QPointF(-60, -60) << SnapDirection::TopLeft;
    QTest::newRow("bottom right") << QPointF(60, 60) << SnapDirection::BottomRight;
    QTest::newRow("on the boundary") << QPointF(48, 0) << SnapDirection::Right;
}

void PlacementTest::directionForDelta()
{
    QFETCH(QPointF, delta);
    QFETCH(SnapDirection, expected);
    QCOMPARE(::directionForDelta(delta, 48), expected);
}

QTEST_GUILESS_MAIN(PlacementTest)

#include "placementtest.moc"
