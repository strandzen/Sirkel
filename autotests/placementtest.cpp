#include "placement.h"

#include <QTest>

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
    for (auto d : {SnapDirection::Left, SnapDirection::Right, SnapDirection::Top, SnapDirection::Bottom, SnapDirection::TopLeft, SnapDirection::TopRight,
                   SnapDirection::BottomLeft, SnapDirection::BottomRight}) {
        const int index = placementFor(d)->ringIndex;
        QVERIFY(index >= 0 && index < 8);
        QVERIFY(!seen.contains(index));
        seen.insert(index);
    }
    QCOMPARE(placementFor(SnapDirection::Maximize)->ringIndex, -1);
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
    QCOMPARE(::directionForDelta(delta, 48, 48), expected);
}

QTEST_GUILESS_MAIN(PlacementTest)

#include "placementtest.moc"
