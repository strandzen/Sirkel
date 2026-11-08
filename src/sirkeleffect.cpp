// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "sirkeleffect.h"
#include "globalshortcuts.h"
#include "hyperkeytrigger.h"
#include "overlaycontroller.h"
#include "rebuildstatus.h"
#include "shortcutactions.h"
#include "sirkel_debug.h"
#include "sirkelconfig.h"

#include <core/output.h>
#include <effect/effecthandler.h>
#include <effect/effectwindow.h>
#include <input_event.h>
#include <window.h>

#include <QAction>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QKeyEvent>
#include <QProcess>
#include <QTimer>

#include <algorithm>
#include <cstdlib>

SirkelEffect::SirkelEffect()
{
    m_trigger = new HyperkeyTrigger(this);
    connect(m_trigger, &HyperkeyTrigger::armRequested, this, &SirkelEffect::arm);

    // Direct per-action shortcuts (System Settings -> Shortcuts -> Sirkel, or
    // Sirkel's Shortcuts page). No defaults: users bind the ones they want.
    for (const ShortcutAction &def : shortcutActions()) {
        auto *action = new QAction(this);
        action->setObjectName(QString::fromLatin1(def.id));
        action->setText(def.text.toString());
        registerGlobalAction(action, {});
        connect(action, &QAction::triggered, this, [this, def]() {
            runShortcutAction(def);
        });
    }

    // Keys that act while the trigger is held. Registered and then released
    // straight away (the QAction goes out of scope): KGlobalAccel keeps an
    // action whose QAction is gone as an inactive entry, listed and
    // editable in System Settings -> Shortcuts but never grabbed, so Enter
    // and Backspace keep working normally outside a session.
    for (const HoldKeyAction &def : holdKeyActions()) {
        QAction action;
        action.setObjectName(QString::fromLatin1(def.id));
        action.setText(def.text.toString());
        registerGlobalAction(&action, def.defaults);
    }

    readExcludedApps();

    m_fillReleaseTimer = new QTimer(this);
    m_fillReleaseTimer->setSingleShot(true);
    m_fillReleaseTimer->setInterval(250);
    connect(m_fillReleaseTimer, &QTimer::timeout, this, [this]() {
        if (m_active) {
            setFilling(false);
        }
    });

    // A KWin update while Sirkel runs: the running KWin keeps Sirkel, the
    // next login won't load it. The folder holding KWin's library is
    // watched (the link itself is replaced, not changed); an update touches
    // many files there, so the check waits until it has settled.
    m_runningKWin = Rebuild::installedKWin();
    m_kwinCheckTimer = new QTimer(this);
    m_kwinCheckTimer->setSingleShot(true);
    m_kwinCheckTimer->setInterval(3000);
    connect(m_kwinCheckTimer, &QTimer::timeout, this, &SirkelEffect::checkKWinUpdate);
    auto *kwinWatcher = new QFileSystemWatcher({QFileInfo(Rebuild::kwinLibraryLink()).absolutePath()}, this);
    connect(kwinWatcher, &QFileSystemWatcher::directoryChanged, m_kwinCheckTimer, qOverload<>(&QTimer::start));

    // Safety net for a missed key release: cancels a session that has seen
    // no input for SessionTimeout seconds, or lets go of the keyboard after a
    // session that ended early.
    m_idleTimer = new QTimer(this);
    m_idleTimer->setSingleShot(true);
    connect(m_idleTimer, &QTimer::timeout, this, [this]() {
        if (m_active) {
            qCWarning(SIRKEL) << "session idle for" << SirkelConfig::sessionTimeout() << "s, cancelling";
            finish(false);
        } else if (m_awaitingRelease) {
            qCWarning(SIRKEL) << "the trigger wasn't let go after" << SirkelConfig::sessionTimeout() << "s, releasing the keyboard";
            m_awaitingRelease = false;
            m_trigger->releaseGrab();
        }
    });

    // The window or screen a session targets can disappear mid-session.
    // Both signals fire while the object is still valid, so cancelling here
    // means nothing touches it afterwards.
    connect(KWin::effects, &KWin::EffectsHandler::windowClosed, this, [this](KWin::EffectWindow *w) {
        m_snapped.remove(w);
        if (m_active && w == m_targetWindow) {
            qCDebug(SIRKEL) << "target window closed, cancelling";
            finish(false);
        }
    });
    connect(KWin::effects, &KWin::EffectsHandler::screenRemoved, this, [this](KWin::LogicalOutput *screen) {
        if (m_active && screen == m_targetScreen) {
            qCDebug(SIRKEL) << "target screen removed, cancelling";
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
    readExcludedApps();
    qCDebug(SIRKEL) << "settings reloaded";
}

void SirkelEffect::arm()
{
    // The held trigger repeats; a running session ignores that.
    if (m_active || m_awaitingRelease) {
        return;
    }
    // Global shortcuts don't fire on the lock screen anyway; this makes sure
    // nothing is grabbed or moved behind it.
    if (KWin::effects->isScreenLocked()) {
        qCDebug(SIRKEL) << "not starting: the screen is locked";
        return;
    }
    KWin::EffectWindow *window = SirkelConfig::targetWindowUnderCursor() ? windowUnderCursor() : nullptr;
    if (SirkelConfig::targetWindowUnderCursor()) {
        qCDebug(SIRKEL) << "window under the cursor at" << KWin::effects->cursorPos() << ":"
                        << (window ? window->windowClass() + QStringLiteral(" \"") + window->caption() + QLatin1Char('"')
                                   : QStringLiteral("(none, using the active window)"));
    }
    if (!window) {
        window = KWin::effects->activeWindow();
    }
    KWin::LogicalOutput *screen = isTargetable(window) ? screenFor(window) : nullptr;
    if (!screen) {
        qCDebug(SIRKEL) << "not starting: no window Sirkel may move" << (window ? window->windowClass() : QStringLiteral("(none)"))
                        << (window ? QStringLiteral("special %1, fullscreen %2, full-screen effect %3, excluded %4, moving %5, movable %6")
                                         .arg(window->isSpecialWindow())
                                         .arg(window->isFullScreen())
                                         .arg(KWin::effects->hasActiveFullScreenEffect())
                                         .arg(isExcluded(window))
                                         .arg(window->isUserMove() || window->isUserResize())
                                         .arg(window->window() && window->window()->isMovable())
                                   : QString());
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
    m_pressedButtons = {};
    m_scrollSteps.reset();
    m_filling = false;
    m_freeSpaces.reset();
    m_fillReleaseTimer->stop();
    m_input.reset();
    m_input.setDeadzone(SirkelConfig::deadzone());
    m_input.setAngleMode(SirkelConfig::pickingMode() == QStringLiteral("angle") ? SirkelConfig::centerRadius() : 0);

    // The usable area's center, not the screen's: that's where the indicator
    // is drawn, and picking is relative to it.
    m_origin = SirkelConfig::indicatorFollowsMouse() ? KWin::effects->cursorPos() : m_area.center();
    // Created on first use, so the QML isn't loaded at KWin startup for a
    // session that never uses Sirkel.
    if (!m_overlay) {
        m_overlay = std::make_unique<OverlayController>();
    }
    m_overlay->begin(m_area);
    m_overlay->applyStyle();
    m_overlay->placeIndicator(m_origin);
    m_edgeOverflow.reset(KWin::effects->cursorPos());
    readHoldKeys();

    KWin::effects->startMouseInterception(this, Qt::CrossCursor);
    restartIdleTimer();
    qCDebug(SIRKEL) << "session started for" << window->windowClass() << window->caption() << "in" << m_area;
}

void SirkelEffect::readHoldKeys()
{
    // Only when KGlobalAccel has saved its file since (a binding changed):
    // one stat() per press instead of parsing the file.
    if (!m_shortcutsFile.reloadIfChanged() && !m_holdKeys.isEmpty()) {
        return;
    }
    m_holdKeys.clear();
    for (const HoldKeyAction &def : holdKeyActions()) {
        m_holdKeys.append({storedShortcuts(QString::fromLatin1(def.id), def.defaults), def.action});
    }
}

void SirkelEffect::checkKWinUpdate()
{
    const QString installed = Rebuild::installedKWin();
    if (installed.isEmpty() || installed == m_runningKWin || installed == m_reportedKWin) {
        return;
    }
    m_reportedKWin = installed;
    qCWarning(SIRKEL) << "KWin was updated from" << m_runningKWin << "to" << installed << "- Sirkel needs a rebuild before the next login";
#if SIRKEL_HAS_REBUILD_HELPER
    // $SIRKEL_TEST_REBUILD_HELPER: the stress test's stand-in.
    const QString test = qEnvironmentVariable("SIRKEL_TEST_REBUILD_HELPER");
    QProcess::startDetached(test.isEmpty() ? QStringLiteral(SIRKEL_REBUILD_EXECUTABLE) : test, {QStringLiteral("--updated")});
#endif
}

void SirkelEffect::readExcludedApps()
{
    m_excluded.clear();
    for (const QString &entry : SirkelConfig::excludedApps()) {
        const QString wanted = entry.trimmed().toLower();
        if (!wanted.isEmpty()) {
            m_excluded.append(wanted);
        }
    }
}

void SirkelEffect::restartIdleTimer()
{
    const int seconds = SirkelConfig::sessionTimeout();
    if (seconds > 0) {
        m_idleTimer->start(seconds * 1000);
    }
}

bool SirkelEffect::isTargetable(KWin::EffectWindow *window) const
{
    // Special windows (desktop, panels, ...) aren't meant to be moved:
    // moving the desktop window shifts the wallpaper. Fullscreen windows
    // (games, video) are left alone entirely for now, and so is everything
    // while Overview, Desktop Grid or a similar full-screen effect is up.
    // A window the user is dragging or resizing is KWin's until let go, and
    // one a window rule holds in place stays there.
    if (!window || window->isSpecialWindow() || window->isFullScreen() || KWin::effects->hasActiveFullScreenEffect() || isExcluded(window)) {
        return false;
    }
    KWin::Window *w = window->window();
    return w && w->isMovable() && !window->isUserMove() && !window->isUserResize();
}

KWin::LogicalOutput *SirkelEffect::screenFor(KWin::EffectWindow *window) const
{
    KWin::LogicalOutput *screen = SirkelConfig::snapToCursorScreen() ? KWin::effects->screenAt(KWin::effects->cursorPos().toPoint()) : nullptr;
    return screen ? screen : window->screen();
}

KWin::EffectWindow *SirkelEffect::windowUnderCursor() const
{
    const QPointF cursor = KWin::effects->cursorPos();
    const QList<KWin::EffectWindow *> stack = KWin::effects->stackingOrder();
    // Topmost first; only regular windows count (no panels, desktop,
    // popups or tooltips).
    for (auto it = stack.crbegin(); it != stack.crend(); ++it) {
        KWin::EffectWindow *w = *it;
        if (w->isDeleted() || w->isMinimized() || w->isHidden() || !w->isOnCurrentDesktop() || !w->isOnCurrentActivity()) {
            continue;
        }
        if ((!w->isNormalWindow() && !w->isDialog()) || !QRectF(w->frameGeometry()).contains(cursor)) {
            continue;
        }
        // Excluded apps are looked through, like panels: some are normal
        // windows over the desktop (dynamicholme), and stopping at one
        // refused the session instead of using the active window.
        if (!isExcluded(w)) {
            return w;
        }
    }
    return nullptr;
}

bool SirkelEffect::isExcluded(KWin::EffectWindow *window) const
{
    KWin::Window *w = window->window();
    if (m_excluded.isEmpty() || !w) {
        return false;
    }
    for (const QString &id : {w->desktopFileName(), w->resourceName(), w->resourceClass()}) {
        if (m_excluded.contains(id.toLower())) {
            return true;
        }
    }
    return false;
}

void SirkelEffect::engage()
{
    if (m_engaged) {
        return;
    }
    m_engaged = true;
    m_overlay->showIndicator();
}

void SirkelEffect::finish(bool apply, bool beforeRelease)
{
    m_idleTimer->stop();
    KWin::effects->stopMouseInterception(this);
    if (beforeRelease) {
        m_awaitingRelease = true;
        restartIdleTimer();
    } else {
        m_trigger->releaseGrab();
    }
    m_active = false;

    if (apply && m_targetWindow && m_direction != SnapDirection::None && m_filling) {
        if (const std::optional<QRectF> fill = fillGeometry()) {
            qCDebug(SIRKEL) << "filling the gap" << placementName(m_direction) << "with" << *fill;
            placeAt(m_targetWindow, *fill);
        } else {
            qCDebug(SIRKEL) << "no free space" << placementName(m_direction) << "- nothing to do";
        }
    } else if (apply && m_targetWindow && m_direction != SnapDirection::None) {
        qCDebug(SIRKEL) << "snapping to" << placementName(m_direction) << "step" << m_sizeStep;
        perform(m_targetWindow, m_area, {WindowAction::Kind::Place, m_direction}, m_sizeStep);
    } else {
        qCDebug(SIRKEL) << "session ended without a snap";
    }
    m_overlay->end();

    m_targetWindow = nullptr;
    m_targetScreen = nullptr;
    m_direction = SnapDirection::None;
    m_engaged = false;
    m_filling = false;
    m_freeSpaces.reset();
    m_fillReleaseTimer->stop();
}

void SirkelEffect::pick(SnapDirection direction)
{
    if (direction != m_direction) {
        m_direction = direction;
        // A scroll-resize step only means something for the pick it was
        // dialed in for.
        m_sizeStep = normalStep(direction);
        m_scrollSteps.reset();
        refreshPreview();
    }
    m_overlay->setIntensity(m_input.intensity());
}

QRectF SirkelEffect::geometryFor(KWin::EffectWindow *window, const QRectF &area, SnapDirection direction, int step)
{
    const QRectF target = alignedToPixels(placementGeometry(direction, area, SirkelConfig::paddingHorizontal(), SirkelConfig::paddingVertical(), step));
    KWin::Window *w = window ? window->window() : nullptr;
    if (!w) {
        return target;
    }
    // The size the window will actually take: its own size if it can't be
    // resized, otherwise the target limited to its minimum and maximum
    // size (as KWin itself will limit it). If that differs from the target,
    // it goes centered on the target and pulled back inside the area, rather
    // than starting at the target's corner and hanging off the screen.
    const QSizeF size = w->isResizable() ? w->constrainFrameSize(target.size()) : QSizeF(w->frameGeometry().size());
    if (qAbs(size.width() - target.width()) < 1 && qAbs(size.height() - target.height()) < 1) {
        return target;
    }
    return alignedToPixels(centeredFixedSize(target, size, area));
}

void SirkelEffect::refreshPreview()
{
    if (m_filling) {
        // No free space there: no outline, and a dimmed indicator says so.
        const std::optional<QRectF> fill = fillGeometry();
        m_overlay->setDimmed(!fill);
        if (fill) {
            m_overlay->showPlacement(m_direction, *fill, m_targetWindow ? QRectF(m_targetWindow->frameGeometry()) : QRectF());
        } else {
            m_overlay->hidePlacement(m_direction);
        }
        return;
    }
    m_overlay->setDimmed(false);
    const QRectF target = geometryFor(m_targetWindow, m_area, m_direction, m_sizeStep);
    m_overlay->showPlacement(m_direction, target, m_targetWindow ? QRectF(m_targetWindow->frameGeometry()) : QRectF());
}

void SirkelEffect::perform(KWin::EffectWindow *window, const QRectF &area, const WindowAction &action, int step)
{
    if (!window) {
        return;
    }
    switch (action.kind) {
    case WindowAction::Kind::Place:
        placeAt(window, geometryFor(window, area, action.direction, step));
        break;
    case WindowAction::Kind::FillGap:
        // Only held while picking (setFilling()), never performed.
        break;
    case WindowAction::Kind::Minimize:
        window->setMinimized(true);
        break;
    case WindowAction::Kind::Restore:
        restore(window, area);
        break;
    }
}

void SirkelEffect::placeAt(KWin::EffectWindow *window, const QRectF &geometry)
{
    rememberSnap(window, geometry);
    moveWindow(window, geometry);
    // A window picked by pointing may not be the active one; give it
    // focus, as you'd expect after moving it.
    if (window != KWin::effects->activeWindow()) {
        KWin::effects->activateWindow(window);
    }
}

void SirkelEffect::setFilling(bool filling)
{
    if (filling == m_filling) {
        return;
    }
    m_filling = filling;
    if (filling) {
        if (!m_freeSpaces) {
            m_freeSpaces = freeSpaces(m_area, fillObstacles());
            qCDebug(SIRKEL) << "fill the gap:" << m_freeSpaces->size() << "free spaces";
        }
        // Held with nothing picked yet: the largest free space anywhere.
        engage();
        if (m_direction == SnapDirection::None) {
            m_input.keyPicked(m_edgeOverflow.current());
            pick(SnapDirection::Maximize);
        }
    }
    refreshPreview();
}

QList<QRectF> SirkelEffect::fillObstacles() const
{
    QList<QRectF> obstacles;
    // A few pixels' slack: a window exactly as big as the one behind it.
    const QRectF target = m_targetWindow ? QRectF(m_targetWindow->frameGeometry()).adjusted(2, 2, -2, -2) : QRectF();
    const QList<KWin::EffectWindow *> stack = KWin::effects->stackingOrder();
    for (KWin::EffectWindow *w : stack) {
        if (w == m_targetWindow || w->isDeleted() || w->isMinimized() || w->isHidden() || !w->isOnCurrentDesktop() || !w->isOnCurrentActivity()) {
            continue;
        }
        if ((!w->isNormalWindow() && !w->isDialog()) || isExcluded(w)) {
            continue;
        }
        const QRectF frame = w->frameGeometry();
        if (frame.intersects(m_area) && !(target.isValid() && frame.contains(target))) {
            obstacles.append(frame);
            qCDebug(SIRKEL) << "fill the gap: blocked by" << w->windowClass() << w->caption() << frame;
        }
    }
    qCDebug(SIRKEL) << "fill the gap: moving" << (m_targetWindow ? m_targetWindow->windowClass() : QString())
                    << (m_targetWindow ? QRectF(m_targetWindow->frameGeometry()) : QRectF()) << "in" << m_area;
    return obstacles;
}

std::optional<QRectF> SirkelEffect::fillGeometry() const
{
    if (!m_freeSpaces || !m_targetWindow) {
        return std::nullopt;
    }
    // Slivers (the gaps between snapped windows) aren't free space.
    const QSizeF minSize(m_area.width() / 8.0, m_area.height() / 8.0);
    const std::optional<QRectF> space = pickFreeSpace(*m_freeSpaces, m_area, m_direction, minSize);
    if (!space) {
        return std::nullopt;
    }
    const QRectF rect = alignedToPixels(paddedFreeSpace(*space, m_area, SirkelConfig::paddingHorizontal(), SirkelConfig::paddingVertical()));
    KWin::Window *w = m_targetWindow->window();
    if (!w) {
        return rect;
    }
    // A window that can't take that size goes centered in the space, as for
    // placements, unless it doesn't fit at all.
    const QSizeF size = w->isResizable() ? w->constrainFrameSize(rect.size()) : QSizeF(w->frameGeometry().size());
    if (size.width() > rect.width() + 1 || size.height() > rect.height() + 1) {
        return std::nullopt;
    }
    if (qAbs(size.width() - rect.width()) < 1 && qAbs(size.height() - rect.height()) < 1) {
        return rect;
    }
    return alignedToPixels(centeredFixedSize(rect, size, rect));
}

void SirkelEffect::restore(KWin::EffectWindow *window, const QRectF &area)
{
    const auto it = m_snapped.constFind(window);
    if (it == m_snapped.constEnd()) {
        qCDebug(SIRKEL) << "nothing to restore for" << window->windowClass();
        return;
    }
    QRectF geometry = it->restore;
    m_snapped.erase(it);
    // Its old place may be on a screen that's gone: then centered here.
    const QPointF center = geometry.center();
    const QList<KWin::LogicalOutput *> screens = KWin::effects->screens();
    const bool onScreen = std::any_of(screens.cbegin(), screens.cend(), [center](KWin::LogicalOutput *screen) {
        return QRectF(screen->geometry()).contains(center);
    });
    if (!onScreen) {
        geometry = alignedToPixels(centeredFixedSize(area, geometry.size().boundedTo(area.size()), area));
    }
    qCDebug(SIRKEL) << "restoring" << window->windowClass() << "to" << geometry;
    moveWindow(window, geometry);
}

namespace
{
// Whether `a` and `b` are the same placement, give or take what clients and
// rounding change about the size Sirkel asked for (e.g. a terminal sizing
// itself in whole character cells).
bool samePlace(const QRectF &a, const QRectF &b)
{
    constexpr qreal tolerance = 16;
    return qAbs(a.left() - b.left()) <= tolerance && qAbs(a.top() - b.top()) <= tolerance && qAbs(a.right() - b.right()) <= tolerance
        && qAbs(a.bottom() - b.bottom()) <= tolerance;
}
}

void SirkelEffect::rememberSnap(KWin::EffectWindow *window, const QRectF &snapped)
{
    KWin::Window *w = window->window();
    if (!w) {
        return;
    }
    const QRectF frame = w->frameGeometry();
    SnapRecord record{frame, snapped};
    const auto it = m_snapped.constFind(window);
    if (it != m_snapped.constEnd() && samePlace(frame, it->snapped)) {
        // Snapped again from a Sirkel placement: keep the size from before
        // the first snap, not the previous placement.
        record.restore = it->restore;
    } else if (w->maximizeMode() != KWin::MaximizeRestore || w->quickTileMode() != KWin::QuickTileMode(KWin::QuickTileFlag::None)) {
        // Maximized or tiled by KWin: its size from before that.
        const QRectF before = w->geometryRestore();
        if (before.isValid()) {
            record.restore = before;
        }
    }
    m_snapped.insert(window, record);
    connect(window, &KWin::EffectWindow::windowStartUserMovedResized, this, &SirkelEffect::onUserMoveStarted, Qt::UniqueConnection);
}

void SirkelEffect::onUserMoveStarted(KWin::EffectWindow *window)
{
    const auto it = m_snapped.find(window);
    KWin::Window *w = window->window();
    if (it == m_snapped.end() || !w || !window->isUserMove()) {
        return;
    }
    const QRectF frame = w->frameGeometry();
    if (!samePlace(frame, it->snapped)) {
        // Moved or resized some other way since Sirkel placed it.
        m_snapped.erase(it);
        return;
    }
    // Its size from before, no larger than its screen and within its own
    // size limits.
    KWin::LogicalOutput *screen = window->screen();
    QSizeF size = it->restore.size();
    if (screen) {
        size = size.boundedTo(KWin::effects->clientArea(KWin::MaximizeArea, screen).size());
    }
    size = w->constrainFrameSize(size);
    if (qAbs(size.width() - frame.width()) < 1 && qAbs(size.height() - frame.height()) < 1) {
        return;
    }
    // Keep the cursor at the same relative spot on the window; KWin keeps
    // the grab point relative to the window's size during the move.
    const QPointF cursor = KWin::effects->cursorPos();
    const QPointF anchor((cursor.x() - frame.x()) / frame.width(), (cursor.y() - frame.y()) / frame.height());
    const QRectF restored(cursor - QPointF(anchor.x() * size.width(), anchor.y() * size.height()), size);
    qCDebug(SIRKEL) << "dragged away: restoring" << window->windowClass() << "to" << restored.size();
    w->moveResize(restored);
}

void SirkelEffect::moveWindow(KWin::EffectWindow *window, const QRectF &geometry)
{
    KWin::Window *w = window ? window->window() : nullptr;
    if (!w || geometry.isEmpty()) {
        return;
    }
    // Out of KWin's own tiling first (quick tiles and custom ones): moved
    // but left in its tile, KWin still treats it as tiled there.
    if (w->requestedTile()) {
        w->requestTile(nullptr);
    }
    w->setMaximize(false, false);
    w->moveResize(geometry);
}

void SirkelEffect::runShortcutAction(const ShortcutAction &shortcut)
{
    // Not while a hyperkey session is running (that session owns the
    // window), and never on the lock screen.
    if (m_active) {
        return;
    }
    if (KWin::effects->isScreenLocked()) {
        qCDebug(SIRKEL) << "shortcut" << shortcut.id << "ignored: the screen is locked";
        return;
    }
    KWin::EffectWindow *window = KWin::effects->activeWindow();
    KWin::LogicalOutput *screen = isTargetable(window) ? screenFor(window) : nullptr;
    qCDebug(SIRKEL) << "shortcut" << shortcut.id << (screen ? "on" : "ignored for") << (window ? window->windowClass() : QStringLiteral("(none)"));
    if (screen) {
        perform(window, KWin::effects->clientArea(KWin::MaximizeArea, screen), shortcut.action, -1);
    }
}

void SirkelEffect::runKeyAction(const WindowAction &action)
{
    perform(m_targetWindow, m_area, action, -1);
    finish(false, true);
}

void SirkelEffect::grabbedKeyboardEvent(QKeyEvent *event)
{
    if (m_awaitingRelease) {
        // The session ended early; only the trigger's release matters now.
        if (m_trigger->isRelease(event)) {
            m_awaitingRelease = false;
            m_idleTimer->stop();
            m_trigger->releaseGrab();
        }
        return;
    }
    if (!m_active) {
        return;
    }
    restartIdleTimer();

    const bool press = event->type() == QEvent::KeyPress;
    const bool release = event->type() == QEvent::KeyRelease;

    // The trigger first: its key may also be a direction key (Meta+Up with
    // the arrows, Meta+A with WASD). Letting go of it ends the session, and
    // its presses (the held key's repeats) pick nothing.
    if (m_trigger->isRelease(event)) {
        finish(true);
        return;
    }
    if (press && m_trigger->isTriggerKey(event->key())) {
        return;
    }
    // The Fill the gap key (Shift): held like a modifier while picking. Not
    // when it's part of the trigger itself, which is held all along.
    if ((press || release) && !m_trigger->isPartOfTrigger(event->key())) {
        for (const auto &[keys, action] : std::as_const(m_holdKeys)) {
            if (action.kind != WindowAction::Kind::FillGap) {
                continue;
            }
            for (const QKeySequence &binding : keys) {
                if (heldKeyOf(binding) == event->key()) {
                    if (event->isAutoRepeat()) {
                        return;
                    }
                    if (press) {
                        m_fillReleaseTimer->stop();
                        setFilling(true);
                    } else if (m_filling) {
                        m_fillReleaseTimer->start();
                    }
                    return;
                }
            }
        }
    }

    const KeyAxis axis = axisForKey(event->key(), SirkelConfig::keybindScheme());
    if (axis != KeyAxis::None && (press || release)) {
        engage();
        if (const auto direction = m_input.keyEvent(axis, press, m_edgeOverflow.current())) {
            // Pressing the direction that's already picked again steps its
            // size, like scrolling down: 1/2, 1/3, 1/4, 2/3 and round again.
            // Only for the four edges for now (a corner takes two keys), and
            // never for a held key's automatic repeats.
            const bool edge = *direction == SnapDirection::Left || *direction == SnapDirection::Right || *direction == SnapDirection::Top
                || *direction == SnapDirection::Bottom;
            if (press && edge && *direction == m_direction && !m_filling) {
                if (!event->isAutoRepeat()) {
                    qCDebug(SIRKEL) << "same direction again: next size";
                    stepSize(-1);
                }
            } else {
                pick(*direction);
            }
        }
        return;
    }

    if (press) {
        // Esc always cancels; the others are rebindable (holdKeyActions()).
        if (event->key() == Qt::Key_Escape) {
            finish(false, true);
            return;
        }
        for (const auto &[keys, action] : std::as_const(m_holdKeys)) {
            for (const QKeySequence &binding : keys) {
                if (action.kind == WindowAction::Kind::FillGap || !matchesKey(binding, event->key(), event->modifiers())) {
                    continue;
                }
                if (action.kind == WindowAction::Kind::Place) {
                    // A placement key (Enter: Maximize) picks like a
                    // direction: the preview shows it and releasing the
                    // trigger applies it, so it can still be changed.
                    engage();
                    m_input.keyPicked(m_edgeOverflow.current());
                    pick(action.direction);
                } else {
                    runKeyAction(action);
                }
                return;
            }
        }
    }
}

void SirkelEffect::pointerMotion(KWin::PointerMotionEvent *event)
{
    if (!m_active) {
        return;
    }
    restartIdleTimer();
    engage();
    // Virtual position: keeps counting movement past a screen edge.
    const QPointF pointer = m_edgeOverflow.update(event->position, event->delta);
    if (const auto direction = m_input.pointerMoved(pointer, pointer - m_origin)) {
        pick(*direction);
    }
}

void SirkelEffect::pointerButton(KWin::PointerButtonEvent *event)
{
    // A click ends the session at once: left snaps, right cancels. On the
    // release, so neither half of the click reaches the window below, and
    // only for a button pressed during the session.
    if (!m_active) {
        return;
    }
    if (event->state != KWin::PointerButtonState::Released) {
        m_pressedButtons |= event->button;
        return;
    }
    if (!(m_pressedButtons & event->button)) {
        return;
    }
    m_pressedButtons &= ~Qt::MouseButtons(event->button);
    if (event->button == Qt::LeftButton) {
        qCDebug(SIRKEL) << "clicked: snapping now";
        finish(true, true);
    } else if (event->button == Qt::RightButton) {
        qCDebug(SIRKEL) << "right-clicked: cancelling";
        finish(false, true);
    }
}

void SirkelEffect::pointerAxis(KWin::PointerAxisEvent *event)
{
    if (!m_active || !m_engaged || event->orientation != Qt::Vertical || m_direction == SnapDirection::None) {
        return;
    }
    // Free space has the size it has: scrolling does nothing while filling.
    if (m_filling) {
        restartIdleTimer();
        return;
    }
    restartIdleTimer();
    if (!SirkelConfig::scrollResizeEnabled()) {
        return;
    }
    const bool touchpad = event->source == KWin::PointerAxisSource::Finger || event->source == KWin::PointerAxisSource::Continuous;
    const int steps = m_scrollSteps.add(event->delta, event->deltaV120, touchpad ? ScrollSteps::Source::Touchpad : ScrollSteps::Source::Wheel);
    // Scrolling up (negative) grows the target. At most a few steps per
    // event, however fast the wheel spins.
    for (int i = 0; i < std::min(std::abs(steps), 4); ++i) {
        stepSize(steps < 0 ? +1 : -1);
    }
}

void SirkelEffect::stepSize(int delta)
{
    const int step = cycleStep(m_direction, m_sizeStep, delta);
    if (step != m_sizeStep) {
        m_sizeStep = step;
        refreshPreview();
    }
}

KWIN_EFFECT_FACTORY(SirkelEffect, "metadata.json")

#include "sirkeleffect.moc"
