// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "overlaycontroller.h"
#include "colorresolver.h"
#include "sirkel_debug.h"
#include "sirkelconfig.h"

#include <effect/effecthandler.h>

#include <KWindowEffects>

#include <QGuiApplication>
#include <QPainterPath>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickView>
#include <QRegion>
#include <QScreen>
#include <QTimer>

#include <algorithm>

namespace
{
// Durations in the QML are authored for OutlineAnimationDuration's default.
constexpr qreal kReferenceDuration = 75.0;
}

OverlayController::OverlayController()
    : m_globals(KSharedConfig::openConfig(QStringLiteral("kdeglobals")))
    , m_view(std::make_unique<QQuickView>(QUrl(QStringLiteral("qrc:/qml/Overlay.qml"))))
{
    m_view->setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus | Qt::WindowTransparentForInput);
    m_view->setColor(Qt::transparent);
    m_view->setResizeMode(QQuickView::SizeRootObjectToView);
    if (m_view->status() == QQuickView::Error) {
        // Without the overlay, snapping still works, just with nothing drawn.
        for (const QQmlError &error : m_view->errors()) {
            qCWarning(SIRKEL) << "overlay failed to load:" << error.toString();
        }
    }

    if (QQuickItem *item = root()) {
        m_outline = item->findChild<QQuickItem *>(QStringLiteral("outlinePart"));
        m_indicator = item->findChild<QQuickItem *>(QStringLiteral("indicatorPart"));
        connect(item, SIGNAL(exitFinished()), this, SLOT(hideNow()));
    }
    if (m_outline) {
        connect(m_outline, SIGNAL(visualRectChanged()), this, SLOT(updateBlur()));
    }

    // In case the exit animation's finished signal never arrives.
    m_hideFallback = new QTimer(this);
    m_hideFallback->setSingleShot(true);
    m_hideFallback->setInterval(2000);
    connect(m_hideFallback, &QTimer::timeout, this, &OverlayController::hideNow);
}

OverlayController::~OverlayController() = default;

QQuickItem *OverlayController::root() const
{
    return m_view->rootObject();
}

void OverlayController::begin(const QRectF &area)
{
    m_hideFallback->stop();
    m_area = area;
    m_outlineShown = false;

    // Without an explicit screen the window keeps the scale of whichever
    // screen it was created on, which renders blurry or oversized on a
    // monitor with a different scale factor.
    const QPoint center = area.center().toPoint();
    for (QScreen *qscreen : QGuiApplication::screens()) {
        if (qscreen->geometry().contains(center)) {
            if (m_view->screen() != qscreen) {
                m_view->setScreen(qscreen);
            }
            break;
        }
    }
    m_view->setGeometry(area.toRect());
    clearBlur();

    if (QQuickItem *item = root()) {
        QMetaObject::invokeMethod(item, "reset");
    }
}

void OverlayController::applyStyle()
{
    reloadColorSources();
    const QColor outlineColor = resolveColor(ColorTarget::Outline);
    m_blurEnabled = SirkelConfig::outlineBlur();
    m_outlineCornerRadius = SirkelConfig::outlineCornerRadius();

    if (QQuickItem *item = root()) {
        // One scale for every animation, following KWin's global animation
        // speed too (0 = instant everywhere).
        const qreal motionScale = SirkelConfig::outlineAnimationDuration() / kReferenceDuration * KWin::effects->animationTimeFactor();
        item->setProperty("motionScale", motionScale);
        item->setProperty("easing", SirkelConfig::easing());
    }
    if (m_indicator) {
        m_indicator->setProperty("cornerRadius", SirkelConfig::indicatorCornerRadius());
        m_indicator->setProperty("ringWidth", SirkelConfig::indicatorRingWidth());
        m_indicator->setProperty("highlightColor", resolveColor(ColorTarget::Pointer));
        m_indicator->setProperty("ringColor", resolveColor(ColorTarget::Ring));
        m_indicator->setProperty("baseOpacity", SirkelConfig::indicatorRingFillOpacity() / 100.0);
    }
    if (m_outline) {
        m_outline->setProperty("borderWidth", SirkelConfig::outlineBorderWidth());
        m_outline->setProperty("borderColor", outlineColor);
        m_outline->setProperty("cornerRadius", m_outlineCornerRadius);
        m_outline->setProperty("fillOpacity", SirkelConfig::outlineFillOpacity());
    }
}

void OverlayController::placeIndicator(const QPointF &center)
{
    if (!m_indicator) {
        return;
    }
    const int size = SirkelConfig::indicatorSize();
    const qreal half = size / 2.0;
    const auto clampAxis = [half](qreal v, qreal lo, qreal hi) {
        return (hi - lo < 2 * half) ? (lo + hi) / 2.0 : std::clamp(v, lo + half, hi - half);
    };
    const QPointF drawCenter(clampAxis(center.x(), m_area.left(), m_area.right()), clampAxis(center.y(), m_area.top(), m_area.bottom()));
    const QPointF topLeft = drawCenter - QPointF(half, half) - m_area.topLeft();
    m_indicator->setProperty("x", topLeft.x());
    m_indicator->setProperty("y", topLeft.y());
    m_indicator->setProperty("width", size);
    m_indicator->setProperty("height", size);
}

void OverlayController::showIndicator()
{
    if (!m_indicator || !SirkelConfig::showIndicator()) {
        return;
    }
    QMetaObject::invokeMethod(m_indicator, "playEntrance");
    if (!m_view->isVisible()) {
        m_view->show();
    }
}

void OverlayController::showPlacement(SnapDirection direction, const QRectF &target, const QRectF &windowFrame)
{
    const Placement *p = placementFor(direction);
    if (m_indicator) {
        // ringIndex before direction: Indicator.qml reads it when direction changes.
        m_indicator->setProperty("ringIndex", p ? p->ringIndex : -1);
        m_indicator->setProperty("direction", placementName(direction));
    }
    if (!m_outline || !p || !SirkelConfig::showOutline()) {
        return;
    }

    const QPointF origin = m_area.topLeft();
    if (!m_outlineShown) {
        m_outlineShown = true;
        // Grow out of the window itself on the first pick of a session.
        const QRectF start = windowFrame.isValid() ? windowFrame.translated(-origin) : QRectF(m_area.center() - origin, QSizeF());
        QMetaObject::invokeMethod(m_outline,
                                  "jumpTo",
                                  Q_ARG(QVariant, start.x()),
                                  Q_ARG(QVariant, start.y()),
                                  Q_ARG(QVariant, start.width()),
                                  Q_ARG(QVariant, start.height()));
        QMetaObject::invokeMethod(m_outline, "playEntrance");
    }
    const QRectF local = target.translated(-origin);
    m_outline->setProperty("targetX", local.x());
    m_outline->setProperty("targetY", local.y());
    m_outline->setProperty("targetWidth", local.width());
    m_outline->setProperty("targetHeight", local.height());

    if (!m_view->isVisible()) {
        m_view->show();
    }
    updateBlur();
}

void OverlayController::setIntensity(qreal intensity)
{
    if (m_indicator) {
        m_indicator->setProperty("intensity", intensity);
    }
}

void OverlayController::end()
{
    // Blur can't fade with the rest, so it goes first.
    m_outlineShown = false;
    clearBlur();
    if (!m_view->isVisible()) {
        return;
    }
    if (QQuickItem *item = root()) {
        QMetaObject::invokeMethod(item, "playExit");
        m_hideFallback->start();
    } else {
        hideNow();
    }
}

void OverlayController::hideNow()
{
    m_hideFallback->stop();
    m_view->hide();
    clearBlur();
}

void OverlayController::clearBlur()
{
    if (!m_blurRegion.isEmpty()) {
        KWindowEffects::enableBlurBehind(m_view.get(), false);
        m_blurRegion = QRegion();
    }
}

void OverlayController::updateBlur()
{
    if (!m_blurEnabled || !m_outlineShown || !m_outline) {
        return;
    }
    // Follows the outline's animated rect, rounded to match its corners.
    const QRectF rect = m_outline->property("visualRect").toRectF();
    QPainterPath path;
    path.addRoundedRect(rect, m_outlineCornerRadius, m_outlineCornerRadius);
    const QRegion region(path.toFillPolygon().toPolygon());
    // Called every animation frame; only tell KWin when the region changed.
    if (region == m_blurRegion) {
        return;
    }
    m_blurRegion = region;
    KWindowEffects::enableBlurBehind(m_view.get(), true, region);
}
