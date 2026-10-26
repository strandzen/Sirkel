// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QKeySequence>
#include <QObject>

#include <functional>

class QAction;
class QKeyEvent;

namespace KWin
{
class Effect;
}

// Everything about how a hyperkey press becomes a session, kept in one place
// so a different trigger (e.g. a touchpad gesture) can be added later without
// touching the session logic.
//
// Activation is a KGlobalAccel action: F24 (Caps Lock via keyd) by default,
// or a Meta combination chosen in Sirkel's settings, also rebindable in
// System Settings -> Shortcuts. Releasing the trigger ends the session. Also owns
// the keyboard grab, which is what delivers that release.
class HyperkeyTrigger : public QObject
{
    Q_OBJECT

public:
    // `busy` reports whether a session is already running, so auto-repeat of
    // the held key doesn't re-trigger.
    HyperkeyTrigger(KWin::Effect *effect, std::function<bool()> busy);
    ~HyperkeyTrigger() override;

    // Whether `event` releases the trigger: its key, or for a combination
    // like Meta+Z any of its modifiers (releasing either key ends the
    // snap). Follows changes made in the settings live, without restarting
    // KWin.
    bool isRelease(const QKeyEvent *event) const;

    // Grabs the keyboard if not already held. Returns false if the grab
    // failed (another effect holds it), in which case no session may start:
    // without the grab the release would never arrive.
    bool ensureGrab();
    void releaseGrab();

Q_SIGNALS:
    void armRequested();

private:
    void onTriggered();
    void setCombo(const QKeySequence &sequence);

    KWin::Effect *m_effect;
    std::function<bool()> m_busy;
    QAction *m_action = nullptr;
    // The trigger's key combination (F24 when none is set).
    QKeyCombination m_combo = QKeyCombination(Qt::Key_F24);
    bool m_grabbed = false;
};
