// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shortcutactions.h"

const QList<ShortcutAction> &shortcutActions()
{
    using K = WindowAction::Kind;
    static const QList<ShortcutAction> actions{
        {"SirkelSnapLeft", kli18n("Sirkel: Snap to left half"), kli18n("Left half"), {K::Place, SnapDirection::Left}},
        {"SirkelSnapRight", kli18n("Sirkel: Snap to right half"), kli18n("Right half"), {K::Place, SnapDirection::Right}},
        {"SirkelSnapTop", kli18n("Sirkel: Snap to top half"), kli18n("Top half"), {K::Place, SnapDirection::Top}},
        {"SirkelSnapBottom", kli18n("Sirkel: Snap to bottom half"), kli18n("Bottom half"), {K::Place, SnapDirection::Bottom}},
        {"SirkelSnapTopLeft", kli18n("Sirkel: Snap to top-left quarter"), kli18n("Top-left quarter"), {K::Place, SnapDirection::TopLeft}},
        {"SirkelSnapTopRight", kli18n("Sirkel: Snap to top-right quarter"), kli18n("Top-right quarter"), {K::Place, SnapDirection::TopRight}},
        {"SirkelSnapBottomLeft", kli18n("Sirkel: Snap to bottom-left quarter"), kli18n("Bottom-left quarter"), {K::Place, SnapDirection::BottomLeft}},
        {"SirkelSnapBottomRight", kli18n("Sirkel: Snap to bottom-right quarter"), kli18n("Bottom-right quarter"), {K::Place, SnapDirection::BottomRight}},
        {"SirkelMaximize", kli18n("Sirkel: Maximize"), kli18n("Maximize"), {K::Place, SnapDirection::Maximize}},
        {"SirkelMinimize", kli18n("Sirkel: Minimize"), kli18n("Minimize"), {K::Minimize}},
        {"SirkelRestore", kli18n("Sirkel: Restore the size from before snapping"), kli18n("Restore size"), {K::Restore}},
    };
    return actions;
}

const QList<HoldKeyAction> &holdKeyActions()
{
    using K = WindowAction::Kind;
    static const QList<HoldKeyAction> actions{
        {"SirkelHoldMaximize",
         kli18n("Sirkel: Maximize (while holding the trigger)"),
         kli18n("Maximize"),
         {QKeySequence(Qt::Key_Return), QKeySequence(Qt::Key_Enter)},
         {K::Place, SnapDirection::Maximize}},
        {"SirkelHoldMinimize", kli18n("Sirkel: Minimize (while holding the trigger)"), kli18n("Minimize"), {QKeySequence(Qt::Key_Backspace)}, {K::Minimize}},
        {"SirkelHoldRestore",
         kli18n("Sirkel: Restore the size from before snapping (while holding the trigger)"),
         kli18n("Restore size"),
         {QKeySequence(Qt::Key_R)},
         {K::Restore}},
    };
    return actions;
}

QList<QKeySequence> parseStoredShortcuts(const QString &keys)
{
    QList<QKeySequence> result;
    if (keys.isEmpty() || keys == QLatin1String("none")) {
        return result;
    }
    for (const QString &part : keys.split(QLatin1Char('\t'), Qt::SkipEmptyParts)) {
        const QKeySequence sequence = QKeySequence::fromString(part, QKeySequence::PortableText);
        if (!sequence.isEmpty()) {
            result.append(sequence);
        }
    }
    return result;
}

bool matchesKey(const QKeySequence &binding, int key, Qt::KeyboardModifiers held)
{
    if (binding.isEmpty() || binding[0].key() != key) {
        return false;
    }
    const Qt::KeyboardModifiers wanted = binding[0].keyboardModifiers() & ~Qt::KeypadModifier;
    return (held & wanted) == wanted;
}

bool isTriggerRelease(QKeyCombination trigger, int key)
{
    if (key == trigger.key()) {
        return true;
    }
    const Qt::KeyboardModifiers mods = trigger.keyboardModifiers();
    return ((mods & Qt::MetaModifier) && (key == Qt::Key_Meta || key == Qt::Key_Super_L || key == Qt::Key_Super_R))
        || ((mods & Qt::ControlModifier) && key == Qt::Key_Control) || ((mods & Qt::AltModifier) && key == Qt::Key_Alt)
        || ((mods & Qt::ShiftModifier) && key == Qt::Key_Shift);
}

QString translatedText(const KLazyLocalizedString &text)
{
    return text.toString();
}
