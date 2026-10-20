#pragma once

#include "placement.h"

#include <KLazyLocalizedString>

#include <QList>
#include <QString>

// The global shortcuts Sirkel registers with KGlobalAccel (component
// "kwin"), shared by the effect (which registers and handles them) and the
// KCM (which edits them). The ids are the stored action names; never rename
// one that has shipped.

// The hyperkey trigger: held for a session, released to snap.
inline constexpr const char *kTriggerActionId = "SirkelSnap";
inline constexpr KLazyLocalizedString kTriggerActionText = kli18n("Sirkel: hold to snap window");

// What an action does to a window: place it, minimize it, or nothing
// (Cancel, for Esc while holding the trigger). Shared by the keys that work
// while holding the trigger and by the direct shortcuts.
struct WindowAction {
    enum class Kind { Place, Minimize, Cancel };
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
