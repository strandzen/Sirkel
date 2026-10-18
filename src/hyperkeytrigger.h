#pragma once

#include <QObject>

#include <functional>

class QAction;
class QKeyEvent;
class QTimer;

namespace KWin
{
class Effect;
}

// Everything about how a hyperkey press becomes a session, kept in one place
// so a different trigger (e.g. a Meta fallback) can be added later without
// touching the session logic.
//
// Activation is a KGlobalAccel action (F24 by default, rebindable in System
// Settings -> Shortcuts). Releasing that same key ends the session. Also owns
// the keyboard grab, since a pending "delay" trigger already needs it before
// a session exists.
class HyperkeyTrigger : public QObject
{
    Q_OBJECT

public:
    // `busy` reports whether a session is already running, so auto-repeat of
    // the held key doesn't re-trigger.
    HyperkeyTrigger(KWin::Effect *effect, std::function<bool()> busy);
    ~HyperkeyTrigger() override;

    // The key whose release ends a session: the action's current shortcut
    // (without modifiers), or F24 if none is set.
    int releaseKey() const;

    // Waiting for the hold delay to elapse, not yet a session.
    bool isPending() const;
    // Feeds a key event while pending. Returns true if it was consumed.
    bool handlePendingKey(QKeyEvent *event);

    // Grabs the keyboard if not already held. Returns false if the grab
    // failed (another effect holds it), in which case no session may start:
    // without the grab the release would never arrive.
    bool ensureGrab();
    void releaseGrab();

Q_SIGNALS:
    void armRequested();

private:
    void onTriggered();
    void cancelPending();

    KWin::Effect *m_effect;
    std::function<bool()> m_busy;
    QAction *m_action = nullptr;
    QTimer *m_delayTimer = nullptr;
    bool m_pending = false;
    bool m_grabbed = false;
};
