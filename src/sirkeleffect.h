#pragma once

#include "snapcalculator.h"

#include <effect/effect.h>

#include <QPointF>
#include <QRectF>

#include <memory>

class QAction;
class QQuickItem;
class QQuickView;
class QTimer;

namespace KWin
{
class EffectWindow;
class LogicalOutput;
struct PointerMotionEvent;
struct PointerAxisEvent;
}

// Loop-style directional window snapping. Hold the hyperkey (Caps Lock,
// remapped to F24 by keyd) to arm the effect, move the mouse (or press an
// arrow key) to pick a direction, release to snap the active window on its
// own screen.
//
// Activation is a single global shortcut (F24 press) handled the ordinary
// KWin-effect way via KGlobalAccel. Everything from press to release is then
// driven by an exclusive keyboard grab + pointer interception
// (effects->grabKeyboard / startMouseInterception).
//
// Visuals are a single plain QQuickWindow (m_overlay, loading Overlay.qml,
// which hosts both the outline preview and the indicator as two Items in
// one scene — see Overlay.qml for why this is one window and not two), not
// a QuickSceneEffect scene. KWin runs its own UI (decorations, tooltips, the
// task switcher) as ordinary internal QQuickWindows composited through the
// normal per-window path, which supports real alpha blending — unlike
// QuickSceneEffect, whose backing surface is always opaque (confirmed live:
// a Window.color="transparent" binding had zero visible effect there).
// Using plain QQuickWindow here is standard public Qt API, not a KWin-
// specific mechanism, so it carries no linking/ABI risk.
class SirkelEffect : public KWin::Effect
{
    Q_OBJECT

public:
    explicit SirkelEffect();
    ~SirkelEffect() override;

    static bool supported();
    bool isActive() const override;

protected:
    void grabbedKeyboardEvent(QKeyEvent *event) override;
    void pointerMotion(KWin::PointerMotionEvent *event) override;
    void pointerAxis(KWin::PointerAxisEvent *event) override;
    void reconfigure(ReconfigureFlags flags) override;

private:
    // The trigger entry point (connected to m_toggleAction) — decides,
    // per TriggerMode, whether to arm() immediately or wait for a delay/
    // double-click gesture to resolve first. See TriggerPhase below.
    void handleTrigger();
    void cancelPendingTrigger();
    void arm();
    void finish(bool applyPendingDirection);
    void engage();
    void updateDirectionFromPointer();
    void setDirection(SnapDirection direction);
    void applyIndicatorStyle();
    void repositionIndicator();
    // Pushes the outline's target rect (and blur region) for m_direction at
    // its current m_sizeStep - shared by setDirection() (a direction just
    // changed) and pointerAxis() (the same direction's size step changed).
    void updateOutlineGeometry();
    // Scroll-resize step away from the normal size for the currently picked
    // direction, applied via adjustSizeStep()/sizeFractionForStep() - see
    // ScrollResizeEnabled's kcfg doc comment for the overall behavior.
    void adjustSizeStep(int delta);
    qreal sizeFractionForStep(SnapDirection direction, int step) const;
    // Tells KWin's Blur effect which part of m_overlay (in the window's own
    // local coordinates, same as what's pushed to targetX/Y/Width/Height)
    // to blur behind, or turns it off — see OutlineBlur's kcfg doc comment
    // for why there's no separate "how much" parameter.
    void applyOutlineBlur(const QRectF &localRect);

    // The two named child Items inside Overlay.qml's scene — see
    // Overlay.qml. Looked up by objectName each time rather than cached,
    // since it's a 2-item tree and this is only called a handful of times
    // per second during an active session at most.
    QQuickItem *outlinePart() const;
    QQuickItem *indicatorPart() const;
    // Shows m_overlay if either part is supposed to be visible, hides it if
    // neither is — the window itself has no independent on/off state of its
    // own any more, since both parts share it. Call after changing either
    // m_outlinePartVisible or m_indicatorPartVisible.
    void updateOverlayVisibility();

    // Hyperkey+<key> actions available while armed. To add a new binding,
    // add a case in grabbedKeyboardEvent's key-action table in the .cpp.
    void actionMaximize();
    void actionMinimize();

    // Not yet a real session (m_targetWindow unset): waiting to see whether
    // this hyperkey press actually resolves into one, per TriggerMode.
    // Idle the rest of the time, including for the entire duration of a
    // real (armed) session — that part is still tracked by m_targetWindow/
    // m_active, same as before this existed.
    enum class TriggerPhase { Idle, PendingDelay, PendingDoubleClick };
    TriggerPhase m_triggerPhase = TriggerPhase::Idle;
    // Whether handleTrigger() already grabbed the keyboard for a pending
    // phase, so arm() (called either directly, or later once that phase
    // resolves) knows not to grab a second time.
    bool m_keyboardAlreadyGrabbed = false;
    QTimer *m_delayTimer = nullptr;
    QTimer *m_doubleClickTimer = nullptr;

    QAction *m_toggleAction = nullptr;
    std::unique_ptr<QQuickView> m_overlay;
    // Whether each part of the shared Overlay.qml scene is currently
    // supposed to be visible — tracked explicitly here because, now that
    // both share one window, "hide the outline" and "hide the window" are
    // no longer the same operation (see updateOverlayVisibility()).
    bool m_outlinePartVisible = false;
    bool m_indicatorPartVisible = false;
    KWin::EffectWindow *m_targetWindow = nullptr;
    KWin::LogicalOutput *m_targetScreen = nullptr;
    // The target screen's usable area for this session, computed once in
    // arm(). m_overlay itself is sized to cover this area for the whole
    // session (not resized per direction change any more) so that moving
    // between directions can be an animated glide of an inner QML rect
    // instead of an instant window move/resize — also reused by
    // repositionIndicator(), finish(), and actionMaximize() instead of each
    // re-querying it.
    QRectF m_outlineArea;
    // The indicator's own drawn center on screen for this session (screen
    // center, or the cursor position at arm() time, per
    // IndicatorFollowsMouse) — set once in repositionIndicator(). Direction
    // is picked from the cursor's *current absolute position relative to
    // this point*, not a running delta accumulated since arm(); otherwise,
    // whenever the indicator is drawn away from the press point (i.e. it's
    // centered on the screen, not following the mouse), the hit-testing
    // would stay anchored to the original press position instead of the
    // indicator everyone can actually see on screen.
    QPointF m_indicatorCenter;
    QPointF m_accumulatedDelta;
    SnapDirection m_direction = SnapDirection::None;
    // Scroll-resize step for m_direction, relative to its normal size - 0
    // is always "normal" (today's half-snap / full-maximize size). Reset to
    // 0 every time m_direction changes to a genuinely different direction,
    // so a scroll adjustment never carries over to the next pick. See
    // sizeFractionForStep() for what each step actually maps to.
    int m_sizeStep = 0;
    bool m_active = false;
    // Holding the hyperkey alone must not show the indicator or grab the
    // screen's attention — only once real input arrives (mouse movement, or
    // a direction key) does the session "engage" and the UI appear. Until
    // then, grabKeyboard()/startMouseInterception() are already in effect
    // (needed to detect that first input at all) but nothing is visible.
    bool m_engaged = false;

    // Currently-held direction keys, composed into a single SnapDirection
    // (see composeDirection() in the .cpp) so holding two at once — e.g.
    // Up+Left — selects a corner instead of only the latest key winning.
    bool m_keyUp = false;
    bool m_keyDown = false;
    bool m_keyLeft = false;
    bool m_keyRight = false;
};
