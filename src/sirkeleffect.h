// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "directioninput.h"
#include "placement.h"
#include "shortcutactions.h"

#include <effect/effect.h>

#include <QKeySequence>
#include <QList>
#include <QPointF>
#include <QRectF>

#include <memory>
#include <utility>

class HyperkeyTrigger;
class QTimer;
class OverlayController;

namespace KWin
{
class EffectWindow;
class LogicalOutput;
struct PointerMotionEvent;
struct PointerAxisEvent;
}

// Loop-style directional window snapping. Hold the hyperkey (Caps Lock,
// remapped to F24 by keyd), move the mouse or press a direction key to pick a
// placement, release to snap the active window there.
//
// The pieces: HyperkeyTrigger decides when a session starts and owns the
// keyboard grab, DirectionInput turns key/pointer input into picks,
// placement.h maps a pick to geometry, OverlayController draws the preview.
// This class is the session state machine tying them together.
class SirkelEffect : public KWin::Effect
{
    Q_OBJECT

public:
    SirkelEffect();
    ~SirkelEffect() override;

    static bool supported();
    bool isActive() const override;

protected:
    void grabbedKeyboardEvent(QKeyEvent *event) override;
    void pointerMotion(KWin::PointerMotionEvent *event) override;
    void pointerAxis(KWin::PointerAxisEvent *event) override;
    void reconfigure(ReconfigureFlags flags) override;

private:
    void arm();
    // Ends the session; snaps to the current pick if `apply`.
    void finish(bool apply);
    // The first real input of a session: shows the indicator.
    void engage();
    void pick(SnapDirection direction);
    void refreshPreview();
    // A key pressed while holding the trigger: Esc, or one of
    // holdKeyActions() (add a row there to bind another key).
    void runKeyAction(const WindowAction &action);
    // Does `action` to `window` within `area` (`step` = scroll-resize step,
    // -1 for the normal size).
    static void perform(KWin::EffectWindow *window, const QRectF &area, const WindowAction &action, int step);
    // Whether Sirkel may act on `window` at all (not special, fullscreen,
    // excluded, or during Overview and similar).
    bool isTargetable(KWin::EffectWindow *window) const;
    // The screen to snap on: the cursor's (SnapToCursorScreen) or the
    // window's own.
    KWin::LogicalOutput *screenFor(KWin::EffectWindow *window) const;
    // Placement geometry for `window` in `area`, with fixed-size windows
    // centered at their own size.
    static QRectF geometryFor(KWin::EffectWindow *window, const QRectF &area, SnapDirection direction, int step);
    static void moveWindow(KWin::EffectWindow *window, const QRectF &geometry);
    // A per-action global shortcut fired: act on the active window directly.
    void runShortcutAction(const ShortcutAction &action);
    // Whether the window's app is in ExcludedApps (matched against its
    // desktop file name and window class, case-insensitively).
    bool isExcluded(KWin::EffectWindow *window) const;
    // Topmost regular window under the cursor on the current desktop, or
    // nullptr (TargetWindowUnderCursor).
    KWin::EffectWindow *windowUnderCursor() const;
    // (Re)starts the SessionTimeout countdown; called on every input.
    void restartIdleTimer();
    // Reads the configured keys for holdKeyActions() into m_holdKeys.
    void readHoldKeys();
    // Where the target window actually ends up for a placement: the
    // placement rect, or for a window that can't be resized, its own size
    // centered in that rect.
    QRectF targetGeometry(SnapDirection direction, int step) const;

    HyperkeyTrigger *m_trigger = nullptr;
    QTimer *m_idleTimer = nullptr;
    std::unique_ptr<OverlayController> m_overlay;
    DirectionInput m_input;
    EdgeOverflow m_edgeOverflow;

    bool m_active = false;
    // Holding the hyperkey alone shows nothing; the session engages on the
    // first mouse movement or direction key.
    bool m_engaged = false;
    KWin::EffectWindow *m_targetWindow = nullptr;
    KWin::LogicalOutput *m_targetScreen = nullptr;
    // Usable area of m_targetScreen, fixed for the session.
    QRectF m_area;
    // Origin for pointer direction picking: the press position, or the
    // screen center when the indicator doesn't spawn at the cursor.
    QPointF m_origin;
    SnapDirection m_direction = SnapDirection::None;
    // Scroll-resize step for m_direction; reset on every new pick.
    int m_sizeStep = 0;
    // holdKeyActions() with their configured keys, read per session.
    QList<std::pair<QList<QKeySequence>, WindowAction>> m_holdKeys;
};
