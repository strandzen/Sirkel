// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shortcutactions.h"

#include <QSet>
#include <QTest>

class ShortcutActionsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void idsAreUnique();
    void parseStored();
    void matching();
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

QTEST_GUILESS_MAIN(ShortcutActionsTest)

#include "shortcutactionstest.moc"
