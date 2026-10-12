#include "sirkeleffect.h"
#include "sirkelconfig.h"

#include <effect/effecthandler.h>
#include <effect/effectwindow.h>
#include <effect/globals.h>
#include <core/output.h>
#include <input_event.h>
#include <window.h>

#include <KConfig>
#include <KConfigGroup>
#include <KGlobalAccel>
#include <KWindowEffects>

#include <QAction>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QPalette>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <array>
#include <cmath>

Q_LOGGING_CATEGORY(SIRKEL, "sirkel")

namespace
{
// Reads a single wallpaper-derived color, per WallpaperColorPath: pywal's
// own default output (colors.color1 of ~/.cache/wal/colors.json) when
// unset, or a user-pointed plain-hex-color file otherwise (covers matugen —
// point one of its templates here — or anything else that can write out a
// plain hex color). Returns an invalid QColor if nothing usable was found,
// so callers can fall back to something that always works.
QColor readWallpaperColor()
{
    const QString configuredPath = SirkelConfig::wallpaperColorPath();
    if (configuredPath.isEmpty()) {
        QFile file(QDir::homePath() + QStringLiteral("/.cache/wal/colors.json"));
        if (!file.open(QIODevice::ReadOnly)) {
            return QColor();
        }
        const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        return QColor(root.value(QStringLiteral("colors")).toObject().value(QStringLiteral("color1")).toString());
    }
    QFile file(configuredPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QColor();
    }
    return QColor(QString::fromUtf8(file.readAll()).trimmed());
}

// A color's kcfg default (OutlineColor/IndicatorPointerColor/
// IndicatorRingColor) is just a static fallback for "manual" mode, not
// itself a live source — the live sources ("theme", "wallpaper") are
// resolved fresh every time instead, per the *Source setting.
// LinkIndicatorColors (checked by the caller, not here) is what makes the
// pointer and ring just reuse the outline's resolved color instead of
// calling this for themselves at all.
enum class ColorTarget { Outline, Pointer, Ring };

QColor resolveColor(ColorTarget target)
{
    QString source;
    switch (target) {
    case ColorTarget::Outline:
        source = SirkelConfig::outlineColorSource();
        break;
    case ColorTarget::Pointer:
        source = SirkelConfig::indicatorPointerColorSource();
        break;
    case ColorTarget::Ring:
        source = SirkelConfig::indicatorRingColorSource();
        break;
    }
    if (source == QStringLiteral("wallpaper")) {
        const QColor wallpaper = readWallpaperColor();
        if (wallpaper.isValid()) {
            return wallpaper;
        }
        // Fall through to the theme accent if the wallpaper file is
        // missing/unparseable — always-valid is more useful than a visibly
        // broken color.
    } else if (source == QStringLiteral("manual")) {
        switch (target) {
        case ColorTarget::Outline:
            return SirkelConfig::outlineColor();
        case ColorTarget::Pointer:
            return SirkelConfig::indicatorPointerColor();
        case ColorTarget::Ring:
            return SirkelConfig::indicatorRingColor();
        }
    }
    return QGuiApplication::palette().color(QPalette::Highlight);
}

std::unique_ptr<QQuickView> makeOverlayWindow(const QUrl &source)
{
    auto view = std::make_unique<QQuickView>(source);
    view->setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool
                   | Qt::WindowDoesNotAcceptFocus | Qt::WindowTransparentForInput);
    view->setColor(Qt::transparent);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    return view;
}

// Which role a physical key plays in direction selection, independent of
// whether it's currently held — lets two keys (e.g. Up+Left) combine into a
// corner instead of only the most-recently-pressed one winning.
enum class KeyAxis { None, Up, Down, Left, Right };

struct KeyBinding {
    Qt::Key key;
    KeyAxis axis;
};

// Which physical keys map to which axis while armed, selected via the
// KeybindScheme setting (KCM: a scheme dropdown, not arbitrary rebinding).
KeyAxis axisForArmedKey(int key, const QString &scheme)
{
    // "none" means mouse-only: no direction key bindings at all, rather
    // than falling back to the "arrows" default below.
    if (scheme == QStringLiteral("none")) {
        return KeyAxis::None;
    }

    static const QHash<QString, std::array<KeyBinding, 4>> kSchemes{
        {QStringLiteral("arrows"),
         {{{Qt::Key_Left, KeyAxis::Left}, {Qt::Key_Right, KeyAxis::Right}, {Qt::Key_Up, KeyAxis::Up}, {Qt::Key_Down, KeyAxis::Down}}}},
        {QStringLiteral("wasd"),
         {{{Qt::Key_A, KeyAxis::Left}, {Qt::Key_D, KeyAxis::Right}, {Qt::Key_W, KeyAxis::Up}, {Qt::Key_S, KeyAxis::Down}}}},
        {QStringLiteral("hjkl"),
         {{{Qt::Key_H, KeyAxis::Left}, {Qt::Key_L, KeyAxis::Right}, {Qt::Key_K, KeyAxis::Up}, {Qt::Key_J, KeyAxis::Down}}}},
    };
    auto it = kSchemes.constFind(scheme);
    if (it == kSchemes.constEnd()) {
        it = kSchemes.constFind(QStringLiteral("arrows"));
    }
    for (const KeyBinding &binding : it.value()) {
        if (binding.key == key) {
            return binding.axis;
        }
    }
    return KeyAxis::None;
}

// Scroll-resize step fractions (see ScrollResizeEnabled's kcfg doc
// comment), in ascending order. Edges/corners cycle through
// kEdgeFractions, anchored on kEdgeNormalIndex (the 0.5/"half" entry) as
// where a fresh direction pick always starts. Maximize uses its own list,
// anchored on its own last entry (1.0, "full") since there's no state
// "larger" than full size. Deliberately a plain fixed list, not a KCM
// setting - adding a step later (e.g. a quarter) is a one-line change
// here plus updating the matching *NormalIndex, not a new UI to build.
const QList<qreal> kEdgeFractions = {1.0 / 3.0, 1.0 / 2.0, 2.0 / 3.0};
const int kEdgeNormalIndex = 1;
const QList<qreal> kMaximizeFractions = {1.0 / 3.0, 1.0 / 2.0, 1.0};
const int kMaximizeNormalIndex = 2;

// Combines the currently-held Up/Down/Left/Right keys into one of the 8
// directions (or None). Opposite keys held together (e.g. Up+Down) cancel
// out on that axis rather than picking one arbitrarily.
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
}

SirkelEffect::SirkelEffect()
    : m_overlay(makeOverlayWindow(QUrl(QStringLiteral("qrc:/qml/Overlay.qml"))))
{
    m_toggleAction = new QAction(this);
    m_toggleAction->setObjectName(QStringLiteral("SirkelSnap"));
    m_toggleAction->setText(QStringLiteral("Sirkel: hold to snap window"));
    KGlobalAccel::self()->setDefaultShortcut(m_toggleAction, {QKeySequence(Qt::Key_F24)});
    KGlobalAccel::self()->setShortcut(m_toggleAction, {QKeySequence(Qt::Key_F24)});
    connect(m_toggleAction, &QAction::triggered, this, &SirkelEffect::handleTrigger);

    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    connect(m_delayTimer, &QTimer::timeout, this, [this]() {
        m_triggerPhase = TriggerPhase::Idle;
        arm();
    });

    m_overlayHideTimer = new QTimer(this);
    m_overlayHideTimer->setSingleShot(true);
    connect(m_overlayHideTimer, &QTimer::timeout, this, [this]() {
        m_overlay->hide();
        KWindowEffects::enableBlurBehind(m_overlay.get(), false);
    });

    // If the window being targeted closes mid-session (hyperkey still held,
    // a direction already chosen), m_targetWindow would otherwise go stale
    // with nothing to notice - finish()/setDirection()/actionMaximize()
    // would then still be touching it right up until release, which is a
    // very plausible explanation for a live report of the display
    // disconnecting/reconnecting under exactly this scenario (a fault deep
    // enough to upset kwin_wayland itself would explain outputs getting
    // re-enumerated on recovery far better than an ordinary Qt-level
    // crash would). windowClosed() fires while the window is still valid
    // (effects wanting to animate the close are expected to refWindow() at
    // this point), so cancelling right here - the same clean path Esc
    // already uses - is enough to make sure nothing touches it again after.
    connect(KWin::effects, &KWin::EffectsHandler::windowClosed, this, [this](KWin::EffectWindow *w) {
        if (m_active && w == m_targetWindow) {
            qCDebug(SIRKEL) << "windowClosed(): target window closed mid-session, cancelling";
            finish(false);
        }
    });

    qCDebug(SIRKEL) << "constructed, build marker rev5-qquickview";
}

SirkelEffect::~SirkelEffect() = default;

bool SirkelEffect::supported()
{
    return KWin::effects->isOpenGLCompositing();
}

bool SirkelEffect::isActive() const
{
    return m_active;
}

void SirkelEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags)
    // The KCM runs in a separate process (System Settings/kcmshell6) with
    // its own SirkelConfig::self() instance; it writes to the same
    // kwinrc, but our already-running copy never re-reads the file on its
    // own. reparseConfiguration() forces KConfig to re-read from disk, then
    // load() repopulates the skeleton's cached values from it — without
    // this, settings changed in the KCM would silently keep using whatever
    // was cached from when this effect's SirkelConfig was first touched.
    SirkelConfig::self()->config()->reparseConfiguration();
    SirkelConfig::self()->load();
    qCDebug(SIRKEL) << "reconfigure(): config reloaded";
}

void SirkelEffect::handleTrigger()
{
    if (m_targetWindow || m_triggerPhase != TriggerPhase::Idle) {
        // F24 auto-repeats at the OS level while held; ignore re-entrant
        // triggers for a session that's already armed, or a trigger gesture
        // that's already pending.
        return;
    }

    // See the identical comment this used to sit next to in arm() — moved
    // here since this is now the single entry point for every trigger mode,
    // and delay needs to know TriggerMode/TriggerDelay *before* arm()
    // itself would otherwise run.
    SirkelConfig::self()->config()->reparseConfiguration();
    SirkelConfig::self()->load();

    const QString mode = SirkelConfig::triggerMode();
    const int delay = SirkelConfig::triggerDelay();
    if (mode == QStringLiteral("delay") && delay > 0) {
        qCDebug(SIRKEL) << "handleTrigger(): pending delay of" << delay << "ms";
        m_triggerPhase = TriggerPhase::PendingDelay;
        m_keyboardAlreadyGrabbed = true;
        KWin::effects->grabKeyboard(this);
        m_delayTimer->start(delay);
        return;
    }

    arm();
}

void SirkelEffect::cancelPendingTrigger()
{
    qCDebug(SIRKEL) << "cancelPendingTrigger(): trigger gesture abandoned";
    m_delayTimer->stop();
    m_triggerPhase = TriggerPhase::Idle;
    if (m_keyboardAlreadyGrabbed) {
        m_keyboardAlreadyGrabbed = false;
        KWin::effects->ungrabKeyboard();
    }
}

void SirkelEffect::arm()
{
    if (m_targetWindow) {
        // Already armed — shouldn't normally be reachable (handleTrigger()
        // already guards re-entrant triggers), but guard here too since
        // arm() can also be called from the delay timer, not just directly.
        return;
    }

    qCDebug(SIRKEL) << "arm() called";
    // Cancel any delayed overlay-hide left pending by a confirm flash from
    // a previous session (see m_overlayHideTimer) - a fast re-trigger must
    // not let that stale timer hide this new session's overlay out from
    // under it a moment later.
    m_overlayHideTimer->stop();
    m_targetWindow = KWin::effects->activeWindow();
    if (!m_targetWindow) {
        qCDebug(SIRKEL) << "arm(): no active window, aborting";
        cancelPendingTrigger();
        return;
    }
    if (m_targetWindow->isSpecialWindow()) {
        // Clicking the bare desktop can make KWin's desktop containment
        // window (the wallpaper layer) the "active window" — calling
        // moveResize() on that isn't a real window move, it visibly shifts
        // the wallpaper and leaves part of the screen black where it used
        // to be. isSpecialWindow() covers the desktop, docks/panels, and
        // similar window types that aren't meant to be moved/resized at
        // all, so refuse to arm against any of them.
        qCDebug(SIRKEL) << "arm(): active window is a special window (desktop/panel/etc), aborting";
        m_targetWindow = nullptr;
        cancelPendingTrigger();
        return;
    }
    // Snapshotted once here, not tracked live if the cursor crosses
    // monitors mid-drag (see SnapToCursorScreen's kcfg doc comment).
    m_targetScreen = SirkelConfig::snapToCursorScreen() ? KWin::effects->screenAt(KWin::effects->cursorPos().toPoint()) : nullptr;
    if (!m_targetScreen) {
        m_targetScreen = m_targetWindow->screen();
    }
    m_accumulatedDelta = QPointF();
    m_direction = SnapDirection::None;
    m_sizeStep = kEdgeNormalIndex;
    m_keyUp = m_keyDown = m_keyLeft = m_keyRight = false;
    m_active = true;
    m_engaged = false;
    m_outlinePartVisible = false;
    m_indicatorPartVisible = false;
    m_outlineEntrancePlayed = false;
    m_indicatorEntrancePlayed = false;
    m_outlineLocalRect = QRectF();

    applyIndicatorStyle();

    if (m_targetScreen) {
        // m_overlay stays fixed for the whole session, covering the
        // screen's entire usable area — only the highlighted rect inside it
        // (targetX/Y/Width/Height) moves, so that move can be animated.
        // Computed before repositionIndicator(), which needs it to convert
        // the indicator's own absolute screen position into this window's
        // local coordinates.
        m_outlineArea = KWin::effects->clientArea(KWin::MaximizeArea, m_targetScreen);
        m_overlay->setGeometry(m_outlineArea.toRect());

        repositionIndicator();
        if (QQuickItem *indicator = indicatorPart()) {
            indicator->setProperty("direction", snapDirectionName(m_direction));
        }
        // Not shown yet — see engage(). Holding the hyperkey by itself
        // should be a no-op; the indicator only appears once the first real
        // input (mouse movement or a direction key) arrives.

        // Reset the outline's own target rect to zero-size at the area's
        // center while still hidden, so the first direction picked this
        // session grows out from the middle instead of gliding in from
        // wherever the previous session last left it.
        if (QQuickItem *outline = outlinePart()) {
            const QPointF center = m_outlineArea.center() - m_outlineArea.topLeft();
            outline->setProperty("targetX", center.x());
            outline->setProperty("targetY", center.y());
            outline->setProperty("targetWidth", 0);
            outline->setProperty("targetHeight", 0);
        }
    }

    // Pending delay/double-click already grabbed the keyboard (needed to
    // watch for the release/second press before arm() was even called) —
    // don't grab it a second time.
    if (!m_keyboardAlreadyGrabbed) {
        KWin::effects->grabKeyboard(this);
    }
    m_keyboardAlreadyGrabbed = false;
    m_triggerPhase = TriggerPhase::Idle;
    KWin::effects->startMouseInterception(this, Qt::CrossCursor);
}

void SirkelEffect::engage()
{
    if (m_engaged) {
        return;
    }
    m_engaged = true;
    if (m_targetScreen && SirkelConfig::showIndicator()) {
        m_indicatorPartVisible = true;
        if (QQuickItem *indicator = indicatorPart()) {
            indicator->setProperty("visible", true);
            if (!m_indicatorEntrancePlayed) {
                m_indicatorEntrancePlayed = true;
                QMetaObject::invokeMethod(indicator, "playEntrance");
            }
        }
        updateOverlayVisibility();
    }
}

void SirkelEffect::finish(bool applyPendingDirection)
{
    qCDebug(SIRKEL) << "finish() applyPendingDirection=" << applyPendingDirection
                          << "m_direction=" << snapDirectionName(m_direction);
    KWin::effects->stopMouseInterception(this);
    KWin::effects->ungrabKeyboard();
    m_outlinePartVisible = false;
    m_indicatorPartVisible = false;
    m_active = false;

    const bool didSnap = applyPendingDirection && m_targetWindow && m_direction != SnapDirection::None;
    if (didSnap) {
        const qreal fraction = sizeFractionForStep(m_direction, m_sizeStep);
        const QRectF target = snapTargetGeometry(m_direction, m_outlineArea, SirkelConfig::paddingHorizontal(), SirkelConfig::paddingVertical(), fraction);
        if (KWin::Window *window = m_targetWindow->window()) {
            window->setMaximize(false, false);
            window->moveResize(target);
            qCDebug(SIRKEL) << "finish(): moveResize done, target=" << target;
        } else {
            qCDebug(SIRKEL) << "finish(): m_targetWindow->window() was null!";
        }
    } else {
        qCDebug(SIRKEL) << "finish(): no snap applied (applyPendingDirection=" << applyPendingDirection
                              << "m_targetWindow=" << m_targetWindow << "m_direction=" << snapDirectionName(m_direction) << ")";
    }

    // A confirm flash needs the overlay to still be visible a moment longer
    // than usual to actually be seen, so delay the hide/blur-teardown
    // instead of doing it immediately in that case (see
    // m_overlayHideTimer). No snap, or the flash is off: hide right away,
    // same as always.
    if (didSnap && SirkelConfig::outlineConfirmFlash()) {
        if (QQuickItem *outline = outlinePart()) {
            QMetaObject::invokeMethod(outline, "playConfirmFlash");
        }
        m_overlayHideTimer->start(360);
    } else {
        m_overlay->hide();
        KWindowEffects::enableBlurBehind(m_overlay.get(), false);
    }

    m_targetWindow = nullptr;
    m_targetScreen = nullptr;
    m_outlineArea = QRectF();
    m_direction = SnapDirection::None;
    m_engaged = false;
}

void SirkelEffect::setDirection(SnapDirection direction)
{
    if (direction == m_direction) {
        return;
    }
    qCDebug(SIRKEL) << "setDirection():" << snapDirectionName(m_direction) << "->" << snapDirectionName(direction);
    m_direction = direction;
    // A scroll-resize step is only meaningful relative to the direction it
    // was dialed in for - picking a different one always starts back at the
    // normal size (confirmed: never persists between interactions).
    m_sizeStep = (direction == SnapDirection::Maximize) ? kMaximizeNormalIndex : kEdgeNormalIndex;

    if (QQuickItem *indicator = indicatorPart()) {
        indicator->setProperty("direction", snapDirectionName(m_direction));
    }
    if (QQuickItem *outline = outlinePart()) {
        // Lets the gradient fill (when enabled) orient itself toward
        // whichever screen edge this direction pulls the window against —
        // see Outline.qml's gradientHorizontal/gradientSolidAtStart.
        outline->setProperty("direction", snapDirectionName(m_direction));
    }

    if (m_direction == SnapDirection::None || !m_targetScreen) {
        m_outlinePartVisible = false;
        if (QQuickItem *outline = outlinePart()) {
            outline->setProperty("visible", false);
        }
        m_outlineLocalRect = QRectF();
        updateBlurRegion();
        updateOverlayVisibility();
        return;
    }

    updateOutlineGeometry();
    if (SirkelConfig::showOutline()) {
        m_outlinePartVisible = true;
        if (QQuickItem *outline = outlinePart()) {
            outline->setProperty("visible", true);
            if (!m_outlineEntrancePlayed) {
                m_outlineEntrancePlayed = true;
                QMetaObject::invokeMethod(outline, "playEntrance");
            }
        }
        updateOverlayVisibility();
    }
}

void SirkelEffect::updateOutlineGeometry()
{
    if (m_direction == SnapDirection::None || !m_targetScreen) {
        return;
    }
    // The outline's own target rect, in m_overlay's local coordinates — the
    // window itself stays fixed (covering m_outlineArea), only this rect
    // moves, so QML's Behavior animations can glide/grow it between
    // directions (or size steps) instead of an instant window move+resize.
    const qreal fraction = sizeFractionForStep(m_direction, m_sizeStep);
    const QRectF target = snapTargetGeometry(m_direction, m_outlineArea, SirkelConfig::paddingHorizontal(), SirkelConfig::paddingVertical(), fraction);
    QRectF local;
    if (QQuickItem *outline = outlinePart()) {
        local = target.translated(-m_outlineArea.topLeft());
        outline->setProperty("targetX", local.x());
        outline->setProperty("targetY", local.y());
        outline->setProperty("targetWidth", local.width());
        outline->setProperty("targetHeight", local.height());
    }
    m_outlineLocalRect = local;
    updateBlurRegion();
}

void SirkelEffect::adjustSizeStep(int delta)
{
    if (!m_active || !m_engaged || m_direction == SnapDirection::None || !m_targetScreen) {
        return;
    }
    if (!SirkelConfig::scrollResizeEnabled()) {
        return;
    }
    const bool isMaximize = m_direction == SnapDirection::Maximize;
    // Corners use the exact same fraction list as edges, just width-only
    // (see snapTargetGeometry's TopLeft/TopRight/BottomLeft/BottomRight
    // cases) — sizeFractionForStep() doesn't need to know the difference,
    // since it already only special-cases Maximize.
    const bool isSingleAxis = m_direction == SnapDirection::Left || m_direction == SnapDirection::Right
        || m_direction == SnapDirection::Top || m_direction == SnapDirection::Bottom
        || m_direction == SnapDirection::TopLeft || m_direction == SnapDirection::TopRight
        || m_direction == SnapDirection::BottomLeft || m_direction == SnapDirection::BottomRight;
    if (!isMaximize && !isSingleAxis) {
        return;
    }

    // Scrolling past either end of the list wraps around to the other
    // instead of stopping there — a cycle, not a clamp.
    const int count = (isMaximize ? kMaximizeFractions : kEdgeFractions).size();
    const int newStep = ((m_sizeStep + delta) % count + count) % count;
    if (newStep == m_sizeStep) {
        return;
    }
    m_sizeStep = newStep;
    updateOutlineGeometry();
}

qreal SirkelEffect::sizeFractionForStep(SnapDirection direction, int step) const
{
    const QList<qreal> &fractions = (direction == SnapDirection::Maximize) ? kMaximizeFractions : kEdgeFractions;
    if (fractions.isEmpty()) {
        return 0.5;
    }
    const int count = fractions.size();
    const int idx = ((step % count) + count) % count;
    return fractions.at(idx);
}

void SirkelEffect::updateBlurRegion()
{
    if (!SirkelConfig::outlineBlur() || !m_outlinePartVisible) {
        KWindowEffects::enableBlurBehind(m_overlay.get(), false);
        return;
    }
    // A plain rectangular region, not following OutlineCornerRadius — the
    // blurred corners end up square even when the outline itself is
    // rounded. Simple and reliable beats a pixel-perfect rounded region
    // here; the mismatch is minor at the small corner radii this effect
    // actually uses.
    KWindowEffects::enableBlurBehind(m_overlay.get(), true, QRegion(m_outlineLocalRect.toRect()));
}

void SirkelEffect::updateDirectionFromPointer()
{
    const SnapDirection candidate = snapDirectionForDelta(m_accumulatedDelta, SirkelConfig::horizontalDeadzone(), SirkelConfig::verticalDeadzone());
    // The center deadzone is itself a live zone (the maximize gesture), not
    // a "no selection yet" state — so unlike the 8 edge/corner directions,
    // returning to center always re-selects it, even after a real direction
    // was picked. (This does mean the hand easing back toward center right
    // before releasing F24 will switch the pick to maximize — a deliberate
    // trade-off for making center reachable at all after leaving it.)
    setDirection(candidate == SnapDirection::None ? SnapDirection::Maximize : candidate);
    updateDirectionIntensity(false);
}

void SirkelEffect::updateDirectionIntensity(bool fullyCommitted)
{
    QQuickItem *indicator = indicatorPart();
    if (!indicator) {
        return;
    }
    if (fullyCommitted) {
        indicator->setProperty("intensity", 1.0);
        return;
    }
    // How far past its own deadzone the current drag is, each axis
    // normalized by its own deadzone so horizontal/vertical are compared
    // fairly even when the two deadzones differ. 1.0 = right at the
    // boundary (just barely past it); grows from there as the drag
    // continues. Mapped to [0.55, 1.0] rather than [0, 1] so the pointer is
    // always clearly visible, never fades to nothing — "more confident"
    // reads as brighter, not as "barely there".
    const qreal hDead = std::max<qreal>(1.0, SirkelConfig::horizontalDeadzone());
    const qreal vDead = std::max<qreal>(1.0, SirkelConfig::verticalDeadzone());
    const qreal raw = std::max(std::abs(m_accumulatedDelta.x()) / hDead, std::abs(m_accumulatedDelta.y()) / vDead);
    const qreal t = std::clamp((raw - 1.0) / 2.0, 0.0, 1.0);
    indicator->setProperty("intensity", 0.55 + 0.45 * t);
}

void SirkelEffect::repositionIndicator()
{
    if (!m_targetScreen) {
        return;
    }
    // Called once at arm() time: either centered on screen, or spawned at
    // wherever the cursor happened to be when the hyperkey was pressed (not
    // live-tracked afterward — confirmed that's the intended behavior).
    // Stored in m_indicatorCenter too: direction hit-testing is anchored to
    // this same point, so it always matches what's actually drawn on screen.
    m_indicatorCenter = SirkelConfig::indicatorFollowsMouse() ? KWin::effects->cursorPos() : m_targetScreen->geometryF().center();
    const int size = SirkelConfig::indicatorSize();
    // Absolute screen position translated into m_overlay's own local
    // coordinates (m_outlineArea must already be set — see arm()), the same
    // translation already used for the outline's own target rect. Plain
    // x/y/width/height — built-in QQuickItem properties, not anything
    // Indicator.qml needs to declare itself.
    const QPointF topLeft = m_indicatorCenter - QPointF(size / 2.0, size / 2.0) - m_outlineArea.topLeft();
    if (QQuickItem *indicator = indicatorPart()) {
        indicator->setProperty("x", topLeft.x());
        indicator->setProperty("y", topLeft.y());
        indicator->setProperty("width", size);
        indicator->setProperty("height", size);
    }
}

void SirkelEffect::applyIndicatorStyle()
{
    // Read fresh each time a session arms, rather than reacting live to
    // config changes mid-session — settings changed in the KCM take effect
    // the next time Sirkel is triggered.
    const QColor outlineColor = resolveColor(ColorTarget::Outline);
    const bool linked = SirkelConfig::linkIndicatorColors();
    const QColor pointerColor = linked ? outlineColor : resolveColor(ColorTarget::Pointer);
    const QColor ringColor = linked ? outlineColor : resolveColor(ColorTarget::Ring);
    const QString easing = SirkelConfig::easing();
    if (QQuickItem *indicator = indicatorPart()) {
        indicator->setProperty("cornerRadius", SirkelConfig::indicatorCornerRadius());
        indicator->setProperty("ringWidth", SirkelConfig::indicatorRingWidth());
        indicator->setProperty("pointerLength", SirkelConfig::indicatorPointerLength());
        indicator->setProperty("showText", SirkelConfig::showIndicatorText());
        indicator->setProperty("highlightColor", pointerColor);
        indicator->setProperty("ringColor", ringColor);
        indicator->setProperty("animationDuration", SirkelConfig::outlineAnimationDuration());
        indicator->setProperty("easing", easing);
        indicator->setProperty("baseOpacity", SirkelConfig::indicatorRingFillOpacity() / 100.0);
    }
    if (QQuickItem *outline = outlinePart()) {
        outline->setProperty("borderWidth", SirkelConfig::outlineBorderWidth());
        outline->setProperty("borderColor", outlineColor);
        outline->setProperty("cornerRadius", SirkelConfig::outlineCornerRadius());
        outline->setProperty("fillOpacity", SirkelConfig::outlineFillOpacity());
        outline->setProperty("animationDuration", SirkelConfig::outlineAnimationDuration());
        outline->setProperty("easing", easing);
        outline->setProperty("confirmFlashEnabled", SirkelConfig::outlineConfirmFlash());
        outline->setProperty("gradientFillEnabled", SirkelConfig::outlineGradientFill());
    }
}

QQuickItem *SirkelEffect::outlinePart() const
{
    QQuickItem *root = m_overlay->rootObject();
    return root ? root->findChild<QQuickItem *>(QStringLiteral("outlinePart")) : nullptr;
}

QQuickItem *SirkelEffect::indicatorPart() const
{
    QQuickItem *root = m_overlay->rootObject();
    return root ? root->findChild<QQuickItem *>(QStringLiteral("indicatorPart")) : nullptr;
}

void SirkelEffect::updateOverlayVisibility()
{
    if (m_outlinePartVisible || m_indicatorPartVisible) {
        if (!m_overlay->isVisible()) {
            m_overlay->show();
        }
    } else if (m_overlay->isVisible()) {
        m_overlay->hide();
    }
}

void SirkelEffect::actionMaximize()
{
    if (!m_targetWindow || !m_targetScreen) {
        return;
    }
    if (KWin::Window *window = m_targetWindow->window()) {
        // Routed through the same padded geometry as the directional snaps
        // (rather than window->setMaximize(true, true), which bypasses
        // padding entirely) so Hyperkey+Enter and the center/maximize
        // gesture both respect PaddingHorizontal/PaddingVertical.
        const qreal fraction = sizeFractionForStep(SnapDirection::Maximize, m_direction == SnapDirection::Maximize ? m_sizeStep : 0);
        const QRectF target = snapTargetGeometry(SnapDirection::Maximize, m_outlineArea, SirkelConfig::paddingHorizontal(), SirkelConfig::paddingVertical(), fraction);
        window->setMaximize(false, false);
        window->moveResize(target);
    }
}

void SirkelEffect::actionMinimize()
{
    if (m_targetWindow) {
        m_targetWindow->setMinimized(true);
    }
}

void SirkelEffect::grabbedKeyboardEvent(QKeyEvent *event)
{
    if (m_triggerPhase != TriggerPhase::Idle) {
        // Not a real session yet — just watching for the gesture that
        // decides whether one starts. Esc always gives up. For "delay",
        // only a release matters (it cancels; the timer itself is what
        // arms if the key's still down when it fires).
        if (event->key() == Qt::Key_Escape && event->type() == QEvent::KeyPress) {
            cancelPendingTrigger();
            return;
        }
        if (event->key() == Qt::Key_F24 && event->type() == QEvent::KeyRelease) {
            cancelPendingTrigger();
        }
        return;
    }

    // Hyperkey+<key> bindings available while armed. Add a row here to
    // extend — the action fires immediately and ends the session without
    // applying a directional snap.
    static const QHash<int, void (SirkelEffect::*)()> keyActions{
        {Qt::Key_Return, &SirkelEffect::actionMaximize},
        {Qt::Key_Enter, &SirkelEffect::actionMaximize},
        {Qt::Key_Backspace, &SirkelEffect::actionMinimize},
    };

    // The configured key scheme (arrows/WASD/HJKL) sets the placement
    // directly. Both press and release are tracked so holding two keys at
    // once (e.g. Up+Left) composes a corner instead of only the latest key
    // winning.
    const KeyAxis axis = axisForArmedKey(event->key(), SirkelConfig::keybindScheme());
    if (axis != KeyAxis::None && (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)) {
        engage();
        const bool pressed = (event->type() == QEvent::KeyPress);
        switch (axis) {
        case KeyAxis::Up:
            m_keyUp = pressed;
            break;
        case KeyAxis::Down:
            m_keyDown = pressed;
            break;
        case KeyAxis::Left:
            m_keyLeft = pressed;
            break;
        case KeyAxis::Right:
            m_keyRight = pressed;
            break;
        case KeyAxis::None:
            break;
        }
        // Sticky, same as the mouse-drag case: only a fresh key PRESS can
        // change the pick — a release must never cancel or change it, full
        // stop. This has to be gated on the event being a press, not just
        // on the recomposed result being non-None: releasing *one* of two
        // keys that composed a corner (e.g. letting go of Left while Up is
        // still held, after Up+Left picked TopLeft) recomposes to a
        // different *valid* direction (Top) — composed != None is true, so
        // without this check it reads as a deliberate new pick and
        // silently downgrades the corner to a single edge, even though
        // nothing was actually pressed. The key state bitmask above still
        // needs updating on every event regardless (release included), so
        // the next real press recomposes from accurate state — only the
        // recompute-and-apply step below is press-only.
        if (pressed) {
            const SnapDirection composed = composeDirection(m_keyUp, m_keyDown, m_keyLeft, m_keyRight);
            if (composed != SnapDirection::None) {
                setDirection(composed);
                // A key press is a deliberate, binary pick - no drag-distance
                // equivalent, so it always reads as fully committed.
                updateDirectionIntensity(true);
            }
        }
        return;
    }

    if (event->type() == QEvent::KeyPress) {
        if (event->key() == Qt::Key_Escape) {
            finish(false);
            return;
        }
        if (const auto action = keyActions.value(event->key())) {
            (this->*action)();
            finish(false);
            return;
        }
    } else if (event->type() == QEvent::KeyRelease && event->key() == Qt::Key_F24) {
        finish(true);
        return;
    }
}

void SirkelEffect::pointerMotion(KWin::PointerMotionEvent *event)
{
    engage();
    // Absolute cursor position relative to the indicator's own drawn
    // center, recomputed fresh every time — not a running sum of each
    // event's relative delta since arm(). A pure delta-accumulator is
    // anchored to wherever the cursor physically was at the press, which
    // silently drifts away from the visible indicator whenever it's drawn
    // somewhere else (i.e. IndicatorFollowsMouse is off and the press
    // didn't happen to land exactly on the screen's center).
    m_accumulatedDelta = event->position - m_indicatorCenter;
    updateDirectionFromPointer();
}

void SirkelEffect::pointerAxis(KWin::PointerAxisEvent *event)
{
    if (event->orientation != Qt::Vertical) {
        return;
    }
    // Sign convention assumed from libinput/wl_pointer (positive = scrolled
    // down/toward the user) to match how every other Wayland app treats
    // wheel direction - not yet live-confirmed on this machine. Flip the
    // comparison below if a live test shows it's backwards.
    adjustSizeStep(event->delta < 0 ? +1 : -1);
}

KWIN_EFFECT_FACTORY(SirkelEffect, "metadata.json")

#include "sirkeleffect.moc"
