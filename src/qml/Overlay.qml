// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

// One window for both the outline preview and the indicator. Two Items in
// one scene stack deterministically (later declared = on top), which two
// separate top-level windows did not. Indicator is declared after Outline so
// it always reads as in front of the preview.
//
// Outline fills the window; Indicator is a sub-rect positioned by
// OverlayController::placeIndicator().
Item {
    id: root
    anchors.fill: parent

    // Emitted once the exit animation is done; the window is hidden then.
    signal exitFinished()

    // For the settings page's live preview, which drives the parts directly.
    readonly property alias outlineItem: outline
    readonly property alias indicatorItem: indicator

    // Multiplier for every animation duration (1 = the authored timings).
    // 0 makes everything instant.
    property real motionScale: 1
    // "fluid" | "linear" | "smooth" | "overshoot", see Easing in the kcfg.
    property string easing: "fluid"
    readonly property int easingType: {
        switch (easing) {
        case "linear": return Easing.Linear;
        case "smooth": return Easing.OutCubic;
        case "overshoot": return Easing.OutBack;
        default: return Easing.BezierSpline;
        }
    }
    // "fluid": Loop's strong ease-out curves - fast start, long gentle
    // settle, no overshoot. Only used when easingType is BezierSpline.
    readonly property var glideCurve: [0.22, 1, 0.47, 1, 1, 1]
    readonly property var slideCurve: [0.22, 1, 0.36, 1, 1, 1]

    function ms(base) {
        return Math.round(base * root.motionScale);
    }

    // Called at the start of every session.
    function reset() {
        exitTimer.stop();
        outline.reset();
        indicator.reset();
    }

    function playExit() {
        outline.playExit();
        indicator.playExit();
        exitTimer.interval = Math.max(1, root.ms(150));
        exitTimer.restart();
    }

    Timer {
        id: exitTimer
        onTriggered: root.exitFinished()
    }

    Outline {
        id: outline
        objectName: "outlinePart"
        anchors.fill: parent
        motionScale: root.motionScale
        easingType: root.easingType
        easingCurve: root.glideCurve
    }

    Indicator {
        id: indicator
        objectName: "indicatorPart"
        motionScale: root.motionScale
        easingType: root.easingType
        easingCurve: root.slideCurve
    }
}
