#pragma once

#include "placement.h"

#include <QPointF>
#include <QStringList>

#include <optional>

// Which role a physical key plays in direction selection.
enum class KeyAxis { None, Up, Down, Left, Right };

// The direction-key presets offered in the KCM ("none" = mouse only).
QStringList keybindSchemes();
KeyAxis axisForKey(int key, const QString &scheme);

// Combines held Up/Down/Left/Right keys into one of the 8 directions (or
// None). Opposite keys on the same axis cancel out.
SnapDirection composeDirection(bool up, bool down, bool left, bool right);

// Turns raw key and pointer input during a session into placement picks.
// Pure logic, no KWin dependency, so it can be unit tested.
//
// Picks are sticky: only a fresh key press or real pointer movement changes
// them, never a key release. The pointer's center deadzone is itself a live
// zone (Maximize), so returning to the center re-selects it.
class DirectionInput
{
public:
    void reset();
    void setDeadzones(qreal horizontal, qreal vertical);

    // A direction key changed state. Returns the new pick, if any. `cursor` is
    // the pointer position at that moment, remembered so small pointer jitter
    // right after a key pick can't override it.
    std::optional<SnapDirection> keyEvent(KeyAxis axis, bool pressed, const QPointF &cursor);

    // The pointer moved. `delta` is the cursor relative to the indicator
    // center. Returns the new pick, if any.
    std::optional<SnapDirection> pointerMoved(const QPointF &cursor, const QPointF &delta);

    // How committed the current pick reads as, in [0.55, 1.0]: how far past
    // its deadzone the pointer is. Key picks are always 1.0.
    qreal intensity() const;

private:
    qreal m_horizontalDeadzone = 48;
    qreal m_verticalDeadzone = 48;
    bool m_up = false;
    bool m_down = false;
    bool m_left = false;
    bool m_right = false;
    // Set by a key pick; the pointer only takes over again once it has moved
    // a full deadzone away from here.
    std::optional<QPointF> m_keyPickCursor;
    QPointF m_lastDelta;
    bool m_lastPickFromKey = false;
};
