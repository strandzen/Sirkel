// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "directioninput.h"

#include <QTest>

class DirectionInputTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // A normal wheel (G502 and most mice): one step per notch, as before.
    void scrollWholeNotches()
    {
        ScrollSteps steps;
        QCOMPARE(steps.add(-15, -120, ScrollSteps::Source::Wheel), -1);
        QCOMPARE(steps.add(-15, -120, ScrollSteps::Source::Wheel), -1);
        QCOMPARE(steps.add(15, 120, ScrollSteps::Source::Wheel), 1);
    }

    // A high-resolution wheel sends a notch as 8 events of 15: one step for
    // all eight, not eight steps.
    void scrollHighResolution()
    {
        ScrollSteps steps;
        int total = 0;
        for (int i = 0; i < 8; ++i) {
            total += steps.add(-1.875, -15, ScrollSteps::Source::Wheel);
        }
        QCOMPARE(total, -1);
        // Half a notch does nothing yet; the other half completes it.
        for (int i = 0; i < 4; ++i) {
            QCOMPARE(steps.add(1.875, 15, ScrollSteps::Source::Wheel), 0);
        }
        QCOMPARE(steps.add(1.875, 15, ScrollSteps::Source::Wheel) + steps.add(1.875, 15, ScrollSteps::Source::Wheel)
                     + steps.add(1.875, 15, ScrollSteps::Source::Wheel) + steps.add(1.875, 15, ScrollSteps::Source::Wheel),
                 1);
    }

    // Turning the other way starts over: a full notch back steps back.
    void scrollReversal()
    {
        ScrollSteps steps;
        for (int i = 0; i < 7; ++i) {
            QCOMPARE(steps.add(-1.875, -15, ScrollSteps::Source::Wheel), 0);
        }
        int back = 0;
        for (int i = 0; i < 8; ++i) {
            back += steps.add(1.875, 15, ScrollSteps::Source::Wheel);
        }
        QCOMPARE(back, 1);
    }

    // A touchpad swipe: many small pixel distances, one step per 50 px.
    void scrollTouchpad()
    {
        ScrollSteps steps;
        int total = 0;
        for (int i = 0; i < 40; ++i) {
            total += steps.add(-3, 0, ScrollSteps::Source::Touchpad);
        }
        QCOMPARE(total, -2); // 120 px
    }

    // No notch information (for example simulated input): one step per
    // event, as before.
    void scrollWithoutNotchInfo()
    {
        ScrollSteps steps;
        QCOMPARE(steps.add(-10, 0, ScrollSteps::Source::Wheel), -1);
        QCOMPARE(steps.add(10, 0, ScrollSteps::Source::Wheel), 1);
        QCOMPARE(steps.add(0, 0, ScrollSteps::Source::Wheel), 0);
    }

    void init();
    void schemes();
    void composeCorners();
    void releaseKeepsCorner();
    void pointerJitterKeepsKeyPick();
    void pointerTakesOverAfterRealMovement();
    void centerIsMaximize();
    void intensity();
    void angleMode();
    void otherKeyPickResistsJitter();
    void edgeOverflowPastLeftEdge();
    void edgeOverflowResetsOnRealMovement();
    void edgeOverflowKeptOnOtherAxis();
    void edgeOverflowIsCapped();

private:
    DirectionInput m_input;
};

void DirectionInputTest::init()
{
    m_input.reset();
    m_input.setDeadzone(48);
}

void DirectionInputTest::schemes()
{
    QCOMPARE(axisForKey(Qt::Key_Left, QStringLiteral("arrows")), KeyAxis::Left);
    QCOMPARE(axisForKey(Qt::Key_W, QStringLiteral("wasd")), KeyAxis::Up);
    QCOMPARE(axisForKey(Qt::Key_J, QStringLiteral("hjkl")), KeyAxis::Down);
    QCOMPARE(axisForKey(Qt::Key_Left, QStringLiteral("none")), KeyAxis::None);
    // Unknown schemes fall back to arrows.
    QCOMPARE(axisForKey(Qt::Key_Right, QStringLiteral("bogus")), KeyAxis::Right);
    QVERIFY(keybindSchemes().contains(QStringLiteral("none")));
}

void DirectionInputTest::composeCorners()
{
    QCOMPARE(composeDirection(true, false, true, false), SnapDirection::TopLeft);
    QCOMPARE(composeDirection(false, true, false, true), SnapDirection::BottomRight);
    QCOMPARE(composeDirection(true, true, true, false), SnapDirection::Left);
    QCOMPARE(composeDirection(true, true, false, false), SnapDirection::None);
}

void DirectionInputTest::releaseKeepsCorner()
{
    const QPointF cursor(500, 500);
    QCOMPARE(m_input.keyEvent(KeyAxis::Up, true, cursor), SnapDirection::Top);
    QCOMPARE(m_input.keyEvent(KeyAxis::Left, true, cursor), SnapDirection::TopLeft);
    QCOMPARE(m_input.keyEvent(KeyAxis::Left, false, cursor), std::nullopt);
    QCOMPARE(m_input.keyEvent(KeyAxis::Up, false, cursor), std::nullopt);
}

void DirectionInputTest::pointerJitterKeepsKeyPick()
{
    const QPointF cursor(500, 500);
    QCOMPARE(m_input.keyEvent(KeyAxis::Left, true, cursor), SnapDirection::Left);
    // A pixel of movement near the center would otherwise pick Maximize.
    QCOMPARE(m_input.pointerMoved(cursor + QPointF(1, 1), QPointF(1, 1)), std::nullopt);
    QCOMPARE(m_input.pointerMoved(cursor + QPointF(-20, 30), QPointF(-20, 30)), std::nullopt);
}

void DirectionInputTest::pointerTakesOverAfterRealMovement()
{
    const QPointF cursor(500, 500);
    m_input.keyEvent(KeyAxis::Left, true, cursor);
    QCOMPARE(m_input.pointerMoved(cursor + QPointF(60, 0), QPointF(60, 0)), SnapDirection::Right);
    // Once the pointer has taken over, small moves count again.
    QCOMPARE(m_input.pointerMoved(cursor + QPointF(2, 0), QPointF(2, 0)), SnapDirection::Maximize);
}

void DirectionInputTest::centerIsMaximize()
{
    QCOMPARE(m_input.pointerMoved(QPointF(1, 1), QPointF(1, 1)), SnapDirection::Maximize);
}

void DirectionInputTest::intensity()
{
    m_input.pointerMoved(QPointF(), QPointF(48, 0));
    QCOMPARE(m_input.intensity(), 0.55);
    m_input.pointerMoved(QPointF(), QPointF(48 * 3, 0));
    QCOMPARE(m_input.intensity(), 1.0);
    m_input.keyEvent(KeyAxis::Up, true, QPointF());
    QCOMPARE(m_input.intensity(), 1.0);
}

void DirectionInputTest::otherKeyPickResistsJitter()
{
    // Enter picks Maximize with the pointer off-center (at a Right pick):
    // small pointer jitter must not turn it back into Right...
    const QPointF cursor(600, 450);
    QCOMPARE(m_input.pointerMoved(cursor, QPointF(100, 0)), SnapDirection::Right);
    m_input.keyPicked(cursor);
    QCOMPARE(m_input.intensity(), 1.0);
    QCOMPARE(m_input.pointerMoved(cursor + QPointF(10, 5), QPointF(110, 5)), std::nullopt);
    // ...but a real move takes over again.
    QCOMPARE(m_input.pointerMoved(cursor + QPointF(0, 80), QPointF(100, 80)), SnapDirection::BottomRight);
}

void DirectionInputTest::angleMode()
{
    m_input.setAngleMode(40);
    QCOMPARE(m_input.pointerMoved(QPointF(), QPointF(10, 10)), SnapDirection::Maximize);
    QCOMPARE(m_input.pointerMoved(QPointF(), QPointF(-35, 35)), SnapDirection::BottomLeft);
    // Jitter guard after a key pick uses the radius in angle mode.
    const QPointF cursor(500, 500);
    QCOMPARE(m_input.keyEvent(KeyAxis::Up, true, cursor), SnapDirection::Top);
    QCOMPARE(m_input.pointerMoved(cursor + QPointF(25, 25), QPointF(25, 25)), std::nullopt);
    QCOMPARE(m_input.pointerMoved(cursor + QPointF(50, 0), QPointF(50, 0)), SnapDirection::Right);
    m_input.setAngleMode(0);
}

void DirectionInputTest::edgeOverflowPastLeftEdge()
{
    // Pressed 5 px from the left edge: the cursor stops at x = 0 but the
    // mouse keeps moving left.
    EdgeOverflow overflow;
    overflow.reset(QPointF(5, 500));
    QPointF p = overflow.update(QPointF(0, 500), QPointF(-5, 0));
    for (int i = 0; i < 10; ++i) {
        p = overflow.update(QPointF(0, 500), QPointF(-6, 0));
    }
    QCOMPARE(p, QPointF(-60, 500));
    // Far enough past the press point to pick Left.
    QCOMPARE(m_input.pointerMoved(p, p - QPointF(5, 500)), SnapDirection::Left);
}

void DirectionInputTest::edgeOverflowResetsOnRealMovement()
{
    EdgeOverflow overflow;
    overflow.reset(QPointF(0, 500));
    overflow.update(QPointF(0, 500), QPointF(-40, 0));
    QCOMPARE(overflow.current(), QPointF(-40, 500));
    // Moving back right: the cursor really moves, the overflow is dropped.
    QCOMPARE(overflow.update(QPointF(8, 500), QPointF(8, 0)), QPointF(8, 500));
}

void DirectionInputTest::edgeOverflowKeptOnOtherAxis()
{
    EdgeOverflow overflow;
    overflow.reset(QPointF(0, 500));
    overflow.update(QPointF(0, 500), QPointF(-40, 0));
    // A purely vertical move while pinned left keeps the horizontal overflow.
    QCOMPARE(overflow.update(QPointF(0, 470), QPointF(0, -30)), QPointF(-40, 470));
}

void DirectionInputTest::edgeOverflowIsCapped()
{
    EdgeOverflow overflow;
    overflow.reset(QPointF(0, 0));
    for (int i = 0; i < 100; ++i) {
        overflow.update(QPointF(0, 0), QPointF(-20, -20));
    }
    QCOMPARE(overflow.current(), QPointF(-EdgeOverflow::kMaxOverflow, -EdgeOverflow::kMaxOverflow));
}

QTEST_GUILESS_MAIN(DirectionInputTest)

#include "directioninputtest.moc"
