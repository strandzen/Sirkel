// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "placement.h"

#include <KLazyLocalizedString>

#include <QKeySequence>
#include <QList>
#include <QString>

// The global shortcuts Sirkel registers with KGlobalAccel, shared by the
// effect (which registers and handles them) and the KCM (which edits them).
// The ids are the stored action names; never rename one that has shipped.

// KGlobalAccel component: Sirkel's own group in System Settings ->
// Shortcuts (before 0.6 the actions lived under "kwin").
inline constexpr const char *kShortcutComponent = "sirkel";
inline constexpr const char *kShortcutComponentName = "Sirkel";

// The hyperkey trigger: held for a session, released to snap.
inline constexpr const char *kTriggerActionId = "SirkelSnap";
inline constexpr KLazyLocalizedString kTriggerActionText = kli18n("Sirkel: hold to snap window");

// What an action does to a window: place it, minimize it, or nothing
// (Cancel, for Esc while holding the trigger). Shared by the keys that work
// while holding the trigger and by the direct shortcuts.
struct WindowAction {
    enum class Kind {
        Place,
        Minimize,
        Cancel
    };
    Kind kind = Kind::Cancel;
    SnapDirection direction = SnapDirection::None;
};

// Direct actions on the active window, no radial menu. No default
// shortcuts; users bind the ones they want.
struct ShortcutAction {
    const char *id;
    // Full name as listed in System Settings -> Shortcuts.
    KLazyLocalizedString text;
    // Short name for Sirkel's own Shortcuts page.
    KLazyLocalizedString label;
    WindowAction action;
};

const QList<ShortcutAction> &shortcutActions();

// Keys that act while the trigger is held (Enter maximizes, Backspace
// minimizes). Listed in System Settings -> Shortcuts so they can be
// rebound, but registered inactive: KGlobalAccel never grabs them, so Enter
// keeps working everywhere else. During a session the effect reads the
// configured keys and matches them itself. Esc (cancel) stays fixed.
struct HoldKeyAction {
    const char *id;
    // Full name as listed in System Settings -> Shortcuts.
    KLazyLocalizedString text;
    // Short name for Sirkel's own Shortcuts page.
    KLazyLocalizedString label;
    QList<QKeySequence> defaults;
    WindowAction action;
};

const QList<HoldKeyAction> &holdKeyActions();

// Parses the keys part of a kglobalshortcutsrc entry (several shortcuts
// separated by tabs, "none" for no shortcut).
QList<QKeySequence> parseStoredShortcuts(const QString &keys);

// Whether a key press matches `binding` (its first key). Extra held
// modifiers are ignored, since the trigger itself may be a combination
// like Meta+Space that's still held; the binding's own modifiers must be
// held. The keypad flag is ignored, so Enter matches either Enter key.
bool matchesKey(const QKeySequence &binding, int key, Qt::KeyboardModifiers held);

// Whether releasing `key` ends a session started with `trigger`: the
// trigger's own key, or for a combination like Meta+Space any of its
// modifiers (releasing either key ends the snap).
bool isTriggerRelease(QKeyCombination trigger, int key);

// A lazy string from this file, translated in Sirkel's own "sirkel" domain.
// KLazyLocalizedString::toString() uses the caller's domain, which for the
// settings is a different one.
QString translatedText(const KLazyLocalizedString &text);
