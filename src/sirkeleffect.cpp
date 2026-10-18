#include "sirkeleffect.h"
#include "hyperkeytrigger.h"
#include "overlaycontroller.h"
#include "sirkelconfig.h"

#include <core/output.h>
#include <effect/effecthandler.h>
#include <effect/effectwindow.h>
#include <input_event.h>
#include <window.h>

#include <QHash>
#include <QKeyEvent>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(SIRKEL, "sirkel")

SirkelEffect::SirkelEffect()
    : m_overlay(std::make_unique<OverlayController>())
{
    m_trigger = new HyperkeyTrigger(this, [this]() {
        return m_active;
    });
    connect(m_trigger, &HyperkeyTrigger::armRequested, this, &SirkelEffect::arm);

    // The window or screen a session targets can disappear mid-session.
    // Both signals fire while the object is still valid, so cancelling here
    // means nothing touches it afterwards.
    connect(KWin::effects, &KWin::EffectsHandler::windowClosed, this, [this](KWin::EffectWindow *w) {
        if (m_active && w == m_targetWindow) {
            finish(false);
        }
    });
    connect(KWin::effects, &KWin::EffectsHandler::screenRemoved, this, [this](KWin::LogicalOutput *screen) {
        if (m_active && screen == m_targetScreen) {
            finish(false);
        }
    });
}

SirkelEffect::~SirkelEffect()
{
    // Unloaded mid-session (e.g. disabled while the hyperkey is held). The
    // keyboard grab is released by m_trigger's own destructor.
    if (m_active) {
        KWin::effects->stopMouseInterception(this);
    }
}

bool SirkelEffect::supported()
{
    // Draws through an ordinary internal QQuickView, not the compositor's
    // GL scene, so any compositing backend works.
    return true;
}

bool SirkelEffect::isActive() const
{
    return m_active;
}

void SirkelEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags)
    // Called over D-Bus by the KCM on save. The KCM runs in another process,
    // so the cached values have to be re-read from kwinrc.
    SirkelConfig::self()->config()->reparseConfiguration();
    SirkelConfig::self()->load();
}

void SirkelEffect::arm()
{
    if (m_active) {
        return;
    }
    KWin::EffectWindow *window = KWin::effects->activeWindow();
    // Special windows (desktop, panels, ...) aren't meant to be moved:
    // moving the desktop window shifts the wallpaper. Fullscreen windows
    // (games, video) are left alone entirely for now.
    if (!window || window->isSpecialWindow() || window->isFullScreen()) {
        m_trigger->releaseGrab();
        return;
    }
    KWin::LogicalOutput *screen = SirkelConfig::snapToCursorScreen() ? KWin::effects->screenAt(KWin::effects->cursorPos().toPoint()) : nullptr;
    if (!screen) {
        screen = window->screen();
    }
    if (!screen) {
        m_trigger->releaseGrab();
        return;
    }
    if (!m_trigger->ensureGrab()) {
        qCWarning(SIRKEL) << "keyboard grab failed, not starting a session";
        return;
    }

    m_targetWindow = window;
    m_targetScreen = screen;
    m_area = KWin::effects->clientArea(KWin::MaximizeArea, screen);
    m_direction = SnapDirection::None;
    m_sizeStep = 0;
    m_active = true;
    m_engaged = false;
    m_input.reset();
    m_input.setDeadzones(SirkelConfig::horizontalDeadzone(), SirkelConfig::verticalDeadzone());

    m_origin = SirkelConfig::indicatorFollowsMouse() ? KWin::effects->cursorPos() : screen->geometryF().center();
    m_overlay->begin(m_area);
    m_overlay->applyStyle();
    m_overlay->placeIndicator(m_origin);

    KWin::effects->startMouseInterception(this, Qt::CrossCursor);
}

void SirkelEffect::engage()
{
    if (m_engaged) {
        return;
    }
    m_engaged = true;
    m_overlay->showIndicator();
}

void SirkelEffect::finish(bool apply)
{
    KWin::effects->stopMouseInterception(this);
    m_trigger->releaseGrab();
    m_active = false;

    const bool snapped = apply && m_targetWindow && m_direction != SnapDirection::None;
    if (snapped) {
        applyPlacement(m_direction, m_sizeStep);
    }
    m_overlay->end(snapped);

    m_targetWindow = nullptr;
    m_targetScreen = nullptr;
    m_direction = SnapDirection::None;
    m_engaged = false;
}

void SirkelEffect::pick(SnapDirection direction)
{
    if (direction != m_direction) {
        m_direction = direction;
        // A scroll-resize step only means something for the pick it was
        // dialed in for.
        m_sizeStep = normalStep(direction);
        refreshPreview();
    }
    m_overlay->setIntensity(m_input.intensity());
}

QRectF SirkelEffect::targetGeometry(SnapDirection direction, int step) const
{
    const QRectF target = placementGeometry(direction, m_area, SirkelConfig::paddingHorizontal(), SirkelConfig::paddingVertical(), step);
    KWin::Window *window = m_targetWindow ? m_targetWindow->window() : nullptr;
    if (window && !window->isResizable()) {
        return centeredFixedSize(target, window->frameGeometry().size(), m_area);
    }
    return target;
}

void SirkelEffect::refreshPreview()
{
    const QRectF target = targetGeometry(m_direction, m_sizeStep);
    m_overlay->showPlacement(m_direction, target, m_targetWindow ? QRectF(m_targetWindow->frameGeometry()) : QRectF());
}

void SirkelEffect::applyPlacement(SnapDirection direction, int step)
{
    KWin::Window *window = m_targetWindow ? m_targetWindow->window() : nullptr;
    if (!window) {
        return;
    }
    window->setMaximize(false, false);
    window->moveResize(targetGeometry(direction, step));
}

void SirkelEffect::runKeyAction(const KeyAction &action)
{
    switch (action.kind) {
    case KeyAction::Kind::Place:
        // Keeps a scroll-resized size if that placement is the current pick.
        applyPlacement(action.placement, action.placement == m_direction ? m_sizeStep : -1);
        break;
    case KeyAction::Kind::Minimize:
        if (m_targetWindow) {
            m_targetWindow->setMinimized(true);
        }
        break;
    case KeyAction::Kind::Cancel:
        break;
    }
    finish(false);
}

void SirkelEffect::grabbedKeyboardEvent(QKeyEvent *event)
{
    if (m_trigger->handlePendingKey(event) || !m_active) {
        return;
    }

    static const QHash<int, KeyAction> keyActions{
        {Qt::Key_Return, {KeyAction::Kind::Place, SnapDirection::Maximize}},
        {Qt::Key_Enter, {KeyAction::Kind::Place, SnapDirection::Maximize}},
        {Qt::Key_Backspace, {KeyAction::Kind::Minimize}},
        {Qt::Key_Escape, {KeyAction::Kind::Cancel}},
    };

    const bool press = event->type() == QEvent::KeyPress;
    const bool release = event->type() == QEvent::KeyRelease;

    const KeyAxis axis = axisForKey(event->key(), SirkelConfig::keybindScheme());
    if (axis != KeyAxis::None && (press || release)) {
        engage();
        if (const auto direction = m_input.keyEvent(axis, press, KWin::effects->cursorPos())) {
            pick(*direction);
        }
        return;
    }

    if (press) {
        const auto it = keyActions.constFind(event->key());
        if (it != keyActions.constEnd()) {
            runKeyAction(it.value());
        }
    } else if (release && event->key() == m_trigger->releaseKey()) {
        finish(true);
    }
}

void SirkelEffect::pointerMotion(KWin::PointerMotionEvent *event)
{
    if (!m_active) {
        return;
    }
    engage();
    if (const auto direction = m_input.pointerMoved(event->position, event->position - m_origin)) {
        pick(*direction);
    }
}

void SirkelEffect::pointerAxis(KWin::PointerAxisEvent *event)
{
    if (!m_active || !m_engaged || event->orientation != Qt::Vertical || m_direction == SnapDirection::None) {
        return;
    }
    if (!SirkelConfig::scrollResizeEnabled()) {
        return;
    }
    // Scrolling up (negative delta) grows the target.
    const int step = cycleStep(m_direction, m_sizeStep, event->delta < 0 ? +1 : -1);
    if (step != m_sizeStep) {
        m_sizeStep = step;
        refreshPreview();
    }
}

KWIN_EFFECT_FACTORY(SirkelEffect, "metadata.json")

#include "sirkeleffect.moc"
