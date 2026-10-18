#include "directioninput.h"

#include <QHash>

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
struct KeyBinding {
    Qt::Key key;
    KeyAxis axis;
};

using Scheme = std::array<KeyBinding, 4>;

const QHash<QString, Scheme> &schemeTable()
{
    static const QHash<QString, Scheme> table{
        {QStringLiteral("arrows"), {{{Qt::Key_Left, KeyAxis::Left}, {Qt::Key_Right, KeyAxis::Right}, {Qt::Key_Up, KeyAxis::Up}, {Qt::Key_Down, KeyAxis::Down}}}},
        {QStringLiteral("wasd"), {{{Qt::Key_A, KeyAxis::Left}, {Qt::Key_D, KeyAxis::Right}, {Qt::Key_W, KeyAxis::Up}, {Qt::Key_S, KeyAxis::Down}}}},
        {QStringLiteral("hjkl"), {{{Qt::Key_H, KeyAxis::Left}, {Qt::Key_L, KeyAxis::Right}, {Qt::Key_K, KeyAxis::Up}, {Qt::Key_J, KeyAxis::Down}}}},
    };
    return table;
}
}

QStringList keybindSchemes()
{
    return {QStringLiteral("arrows"), QStringLiteral("wasd"), QStringLiteral("hjkl"), QStringLiteral("none")};
}

KeyAxis axisForKey(int key, const QString &scheme)
{
    if (scheme == QStringLiteral("none")) {
        return KeyAxis::None;
    }
    const auto &table = schemeTable();
    auto it = table.constFind(scheme);
    if (it == table.constEnd()) {
        it = table.constFind(QStringLiteral("arrows"));
    }
    for (const KeyBinding &binding : it.value()) {
        if (binding.key == key) {
            return binding.axis;
        }
    }
    return KeyAxis::None;
}

SnapDirection composeDirection(bool up, bool down, bool left, bool right)
{
    const bool vertical = up != down;
    const bool horizontal = left != right;
    if (vertical && horizontal) {
        if (up) {
            return left ? SnapDirection::TopLeft : SnapDirection::TopRight;
        }
        return left ? SnapDirection::BottomLeft : SnapDirection::BottomRight;
    }
    if (vertical) {
        return up ? SnapDirection::Top : SnapDirection::Bottom;
    }
    if (horizontal) {
        return left ? SnapDirection::Left : SnapDirection::Right;
    }
    return SnapDirection::None;
}

void DirectionInput::reset()
{
    m_up = m_down = m_left = m_right = false;
    m_keyPickCursor.reset();
    m_lastDelta = QPointF();
    m_lastPickFromKey = false;
}

void DirectionInput::setDeadzones(qreal horizontal, qreal vertical)
{
    m_horizontalDeadzone = horizontal;
    m_verticalDeadzone = vertical;
}

std::optional<SnapDirection> DirectionInput::keyEvent(KeyAxis axis, bool pressed, const QPointF &cursor)
{
    switch (axis) {
    case KeyAxis::Up:
        m_up = pressed;
        break;
    case KeyAxis::Down:
        m_down = pressed;
        break;
    case KeyAxis::Left:
        m_left = pressed;
        break;
    case KeyAxis::Right:
        m_right = pressed;
        break;
    case KeyAxis::None:
        return std::nullopt;
    }
    // Only a press can change the pick. Releasing one key of a corner pair
    // (Left of Up+Left) would otherwise recompose to an edge (Top).
    if (!pressed) {
        return std::nullopt;
    }
    const SnapDirection composed = composeDirection(m_up, m_down, m_left, m_right);
    if (composed == SnapDirection::None) {
        return std::nullopt;
    }
    m_keyPickCursor = cursor;
    m_lastPickFromKey = true;
    return composed;
}

std::optional<SnapDirection> DirectionInput::pointerMoved(const QPointF &cursor, const QPointF &delta)
{
    if (m_keyPickCursor) {
        const QPointF moved = cursor - *m_keyPickCursor;
        if (std::abs(moved.x()) < m_horizontalDeadzone && std::abs(moved.y()) < m_verticalDeadzone) {
            return std::nullopt;
        }
        m_keyPickCursor.reset();
    }
    m_lastDelta = delta;
    m_lastPickFromKey = false;
    const SnapDirection candidate = directionForDelta(delta, m_horizontalDeadzone, m_verticalDeadzone);
    return candidate == SnapDirection::None ? SnapDirection::Maximize : candidate;
}

qreal DirectionInput::intensity() const
{
    if (m_lastPickFromKey) {
        return 1.0;
    }
    // Each axis normalized by its own deadzone; 1.0 = right at the boundary.
    const qreal h = std::max<qreal>(1.0, m_horizontalDeadzone);
    const qreal v = std::max<qreal>(1.0, m_verticalDeadzone);
    const qreal raw = std::max(std::abs(m_lastDelta.x()) / h, std::abs(m_lastDelta.y()) / v);
    const qreal t = std::clamp((raw - 1.0) / 2.0, 0.0, 1.0);
    return 0.55 + 0.45 * t;
}
