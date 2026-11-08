// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shortcutactions.h"

#include <QSet>
#include <QTest>

class ShortcutActionsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // The Fill the gap key: a lone modifier, however the shortcut editor
    // and KGlobalAccel's file store it.
    void heldKeyOfModifierOnly()
    {
        QCOMPARE(heldKeyOf(QKeySequence(Qt::Key_Shift)), int(Qt::Key_Shift));
        QCOMPARE(heldKeyOf(QKeySequence(QKeyCombination(Qt::ShiftModifier, Qt::Key_unknown))), int(Qt::Key_Shift));
        QCOMPARE(heldKeyOf(QKeySequence(QKeyCombination(Qt::ControlModifier, Qt::Key(0)))), int(Qt::Key_Control));
        QCOMPARE(heldKeyOf(QKeySequence(Qt::Key_Space)), int(Qt::Key_Space));
        QCOMPARE(heldKeyOf(QKeySequence()), 0);
        // Round trip through the stored text.
        const QString stored = QKeySequence(Qt::Key_Shift).toString(QKeySequence::PortableText);
        QCOMPARE(heldKeyOf(parseStoredShortcuts(stored).value(0)), int(Qt::Key_Shift));
    }

    // A trigger with Shift in it holds Shift all along.
    void fillKeyInTrigger()
    {
        QVERIFY(isTriggerRelease(QKeyCombination(Qt::MetaModifier | Qt::ShiftModifier, Qt::Key_Z), Qt::Key_Shift));
        QVERIFY(!isTriggerRelease(QKeyCombination(Qt::Key_F24), Qt::Key_Shift));
    }

    void idsAreUnique();
    void parseStored();
    void matching();
    void triggerRelease();
};

void ShortcutActionsTest::idsAreUnique()
{
    QSet<QString> ids{QString::fromLatin1(kTriggerActionId)};
    int count = 1;
    for (const ShortcutAction &action : shortcutActions()) {
        ids.insert(QString::fromLatin1(action.id));
        ++count;
    }
    for (const HoldKeyAction &action : holdKeyActions()) {
        ids.insert(QString::fromLatin1(action.id));
        ++count;
    }
    QCOMPARE(ids.size(), count);
}

void ShortcutActionsTest::parseStored()
{
    QCOMPARE(parseStoredShortcuts(QStringLiteral("none")), QList<QKeySequence>());
    QCOMPARE(parseStoredShortcuts(QString()), QList<QKeySequence>());
    QCOMPARE(parseStoredShortcuts(QStringLiteral("Return")), QList<QKeySequence>{QKeySequence(Qt::Key_Return)});
    // Several shortcuts are separated by tabs.
    QCOMPARE(parseStoredShortcuts(QStringLiteral("Return\tEnter")), (QList<QKeySequence>{QKeySequence(Qt::Key_Return), QKeySequence(Qt::Key_Enter)}));
    QCOMPARE(parseStoredShortcuts(QStringLiteral("Ctrl+M")), QList<QKeySequence>{QKeySequence(Qt::CTRL | Qt::Key_M)});
}

void ShortcutActionsTest::matching()
{
    const QKeySequence enter(Qt::Key_Return);
    QVERIFY(matchesKey(enter, Qt::Key_Return, Qt::NoModifier));
    // The trigger's own modifier (e.g. Meta of Meta+Space) may still be held.
    QVERIFY(matchesKey(enter, Qt::Key_Return, Qt::MetaModifier));
    QVERIFY(!matchesKey(enter, Qt::Key_Backspace, Qt::NoModifier));
    // A binding's own modifiers must be held.
    const QKeySequence ctrlM(Qt::CTRL | Qt::Key_M);
    QVERIFY(matchesKey(ctrlM, Qt::Key_M, Qt::ControlModifier | Qt::MetaModifier));
    QVERIFY(!matchesKey(ctrlM, Qt::Key_M, Qt::NoModifier));
    // Keypad Enter matches a plain Enter binding, and the reverse.
    QVERIFY(matchesKey(QKeySequence(Qt::Key_Enter), Qt::Key_Enter, Qt::KeypadModifier));
    QVERIFY(matchesKey(QKeySequence(Qt::KeypadModifier | Qt::Key_Enter), Qt::Key_Enter, Qt::NoModifier));
    QVERIFY(!matchesKey(QKeySequence(), Qt::Key_Return, Qt::NoModifier));
}

void ShortcutActionsTest::triggerRelease()
{
    // A single key (F24 from Caps Lock): only that key.
    const QKeyCombination f24(Qt::Key_F24);
    QVERIFY(isTriggerRelease(f24, Qt::Key_F24));
    QVERIFY(!isTriggerRelease(f24, Qt::Key_Meta));
    QVERIFY(!isTriggerRelease(f24, Qt::Key_Left));

    // A combination: its key or any of its modifiers, whichever way the
    // Meta key is reported.
    const QKeyCombination metaSpace(Qt::META | Qt::Key_Space);
    QVERIFY(isTriggerRelease(metaSpace, Qt::Key_Space));
    QVERIFY(isTriggerRelease(metaSpace, Qt::Key_Meta));
    QVERIFY(isTriggerRelease(metaSpace, Qt::Key_Super_L));
    QVERIFY(isTriggerRelease(metaSpace, Qt::Key_Super_R));
    // Modifiers that aren't part of it, and direction keys, don't count.
    QVERIFY(!isTriggerRelease(metaSpace, Qt::Key_Control));
    QVERIFY(!isTriggerRelease(metaSpace, Qt::Key_Shift));
    QVERIFY(!isTriggerRelease(metaSpace, Qt::Key_Return));

    const QKeyCombination ctrlAltX(Qt::CTRL | Qt::ALT | Qt::Key_X);
    QVERIFY(isTriggerRelease(ctrlAltX, Qt::Key_Control));
    QVERIFY(isTriggerRelease(ctrlAltX, Qt::Key_Alt));
    QVERIFY(isTriggerRelease(ctrlAltX, Qt::Key_X));
    QVERIFY(!isTriggerRelease(ctrlAltX, Qt::Key_Meta));

    const QKeyCombination shiftF12(Qt::SHIFT | Qt::Key_F12);
    QVERIFY(isTriggerRelease(shiftF12, Qt::Key_Shift));
    QVERIFY(!isTriggerRelease(shiftF12, Qt::Key_Alt));
}

QTEST_GUILESS_MAIN(ShortcutActionsTest)

#include "shortcutactionstest.moc"
