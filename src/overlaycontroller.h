// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "placement.h"

#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QRegion>

#include <memory>

class QQuickItem;
class QQuickView;
class QTimer;

// Owns the overlay window (Overlay.qml: the outline preview and the
// indicator as two Items in one scene, so their stacking is deterministic)
// and everything pushed into it. Takes absolute screen coordinates and
// translates them into the window's own local space.
//
// A plain QQuickView rather than a QuickSceneEffect: KWin composites its
// internal QQuickWindows with real alpha blending, while a QuickSceneEffect's
// surface is opaque.
class OverlayController : public QObject
{
    Q_OBJECT

public:
    OverlayController();
    ~OverlayController() override;

    // Starts a session covering `area` (the usable area of the target
    // screen). Cancels any exit animation still running from the previous
    // session.
    void begin(const QRectF &area);
    // Pushes all style settings (colors, sizes, motion) from SirkelConfig.
    void applyStyle();
    // Centers the indicator on `center`, pushed inward as needed so it stays
    // fully inside the area.
    void placeIndicator(const QPointF &center);
    void showIndicator();
    // `windowFrame` is where the preview grows out of on the session's first
    // pick (the target window's current frame).
    void showPlacement(SnapDirection direction, const QRectF &target, const QRectF &windowFrame);
    void setIntensity(qreal intensity);
    // Plays the exit, then hides.
    void end();

private Q_SLOTS:
    // Connected by name to QML signals (visualRectChanged/exitFinished).
    void updateBlur();
    void hideNow();

private:
    QQuickItem *root() const;

    std::unique_ptr<QQuickView> m_view;
    QPointer<QQuickItem> m_outline;
    QPointer<QQuickItem> m_indicator;
    QTimer *m_hideFallback = nullptr;
    QRectF m_area;
    bool m_outlineShown = false;
    bool m_blurEnabled = false;
    int m_outlineCornerRadius = 0;
    // Last region sent to KWin's blur, to skip unchanged per-frame updates.
    QRegion m_blurRegion;
};
