// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gapfinder.h"

#include <QTest>

// A 1600 x 900 screen, as in the stress test.
static const QRectF kArea(0, 0, 1600, 900);
static const QSizeF kMin(1600 / 8.0, 900 / 8.0);

class GapFinderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyScreenIsOneSpace()
    {
        const QList<QRectF> spaces = freeSpaces(kArea, {});
        QCOMPARE(spaces.size(), 1);
        QCOMPARE(spaces.first(), kArea);
        // It reaches into every half, so any direction fills it.
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::Maximize, kMin), kArea);
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::Left, kMin), kArea);
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::BottomRight, kMin), kArea);
    }

    // The original example: Left at 1/4 and Right at 1/2 leave a quarter
    // between them, and Shift + Left fills it.
    void quarterBetweenWindows()
    {
        const QList<QRectF> windows{QRectF(0, 0, 400, 900), QRectF(800, 0, 800, 900)};
        const QList<QRectF> spaces = freeSpaces(kArea, windows);
        QCOMPARE(spaces.size(), 1);
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::Left, kMin), QRectF(400, 0, 400, 900));
        QVERIFY(!pickFreeSpace(spaces, kArea, SnapDirection::Right, kMin));
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::Maximize, kMin), QRectF(400, 0, 400, 900));
    }

    // Three quarters taken: the fourth is the space.
    void lastQuarter()
    {
        const QList<QRectF> windows{QRectF(0, 0, 800, 450), QRectF(800, 0, 800, 450), QRectF(0, 450, 800, 450)};
        const QList<QRectF> spaces = freeSpaces(kArea, windows);
        QCOMPARE(spaces.size(), 1);
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::BottomRight, kMin), QRectF(800, 450, 800, 450));
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::Right, kMin), QRectF(800, 450, 800, 450));
        QVERIFY(!pickFreeSpace(spaces, kArea, SnapDirection::TopLeft, kMin));
    }

    // One window in the top-left quarter leaves an L: two overlapping
    // maximal spaces, the right half and the bottom half.
    void lShapedSpace()
    {
        const QList<QRectF> spaces = freeSpaces(kArea, {QRectF(0, 0, 800, 450)});
        QCOMPARE(spaces.size(), 2);
        QVERIFY(spaces.contains(QRectF(800, 0, 800, 900)));
        QVERIFY(spaces.contains(QRectF(0, 450, 1600, 450)));
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::Right, kMin), QRectF(800, 0, 800, 900));
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::Bottom, kMin), QRectF(0, 450, 1600, 450));
        // The only free space reaching left is the bottom half.
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::Left, kMin), QRectF(0, 450, 1600, 450));
        QVERIFY(!pickFreeSpace(spaces, kArea, SnapDirection::TopLeft, kMin));
    }

    void overlappingWindows()
    {
        const QList<QRectF> windows{QRectF(0, 0, 600, 900), QRectF(400, 0, 400, 900)};
        const QList<QRectF> spaces = freeSpaces(kArea, windows);
        QCOMPARE(spaces.size(), 1);
        QCOMPARE(spaces.first(), QRectF(800, 0, 800, 900));
    }

    // Slivers (snapped windows' gaps) aren't free space.
    void tooSmallIsNothing()
    {
        const QList<QRectF> windows{QRectF(12, 12, 776, 876), QRectF(812, 12, 776, 876)};
        const QList<QRectF> spaces = freeSpaces(kArea, windows);
        QVERIFY(!spaces.isEmpty());
        for (SnapDirection d : {SnapDirection::Left, SnapDirection::Right, SnapDirection::Top, SnapDirection::Bottom, SnapDirection::Maximize}) {
            QVERIFY(!pickFreeSpace(spaces, kArea, d, kMin));
        }
    }

    void fullScreenIsNothing()
    {
        QVERIFY(freeSpaces(kArea, {kArea}).isEmpty());
        QVERIFY(!pickFreeSpace(freeSpaces(kArea, {kArea}), kArea, SnapDirection::Maximize, kMin));
    }

    // Windows reaching past the screen are cut to it.
    void windowsClippedToTheArea()
    {
        const QList<QRectF> spaces = freeSpaces(kArea, {QRectF(-500, -100, 900, 1200)});
        QCOMPARE(spaces.size(), 1);
        QCOMPARE(spaces.first(), QRectF(400, 0, 1200, 900));
    }

    // Two equal spaces on the left: the one nearer the left edge wins.
    void tieGoesToTheEdge()
    {
        const QList<QRectF> windows{QRectF(200, 0, 100, 900), QRectF(500, 0, 1100, 900)};
        const QList<QRectF> spaces = freeSpaces(kArea, windows);
        QCOMPARE(pickFreeSpace(spaces, kArea, SnapDirection::Left, QSizeF(100, 100)), QRectF(0, 0, 200, 900));
    }

    // Padding lines up with snapped neighbours: full at the screen edge,
    // half against a window (the window has the other half).
    void padding()
    {
        // The quarter between two padded snapped windows (padding 12).
        const QRectF space(394, 0, 412, 900);
        QCOMPARE(paddedFreeSpace(space, kArea, 12, 12), QRectF(400, 12, 400, 876));
        QCOMPARE(paddedFreeSpace(kArea, kArea, 12, 8), QRectF(12, 8, 1576, 884));
    }

    // A padded gap ending a few pixels past the middle isn't on that side.
    void sliverPastTheMiddle()
    {
        const QList<QRectF> windows{QRectF(12, 12, 382, 876), QRectF(806, 12, 782, 876)};
        const QList<QRectF> spaces = freeSpaces(kArea, windows);
        QVERIFY(pickFreeSpace(spaces, kArea, SnapDirection::Left, kMin));
        QVERIFY(!pickFreeSpace(spaces, kArea, SnapDirection::Right, kMin));
    }
};

QTEST_GUILESS_MAIN(GapFinderTest)

#include "gapfindertest.moc"
