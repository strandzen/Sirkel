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

    // Multiplier for every animation duration (1 = the authored timings).
    // 0 makes everything instant.
    property real motionScale: 1
    // "linear" | "smooth" | "overshoot", see Easing in the kcfg.
    property string easing: "overshoot"
    readonly property int easingType: {
        switch (easing) {
        case "linear": return Easing.Linear;
        case "smooth": return Easing.OutCubic;
        default: return Easing.OutBack;
        }
    }

    function ms(base) {
        return Math.round(base * root.motionScale);
    }

    // Called at the start of every session.
    function reset() {
        exitTimer.stop();
        outline.reset();
        indicator.reset();
    }

    function playExit(confirmed) {
        const hold = outline.playExit(confirmed);
        indicator.playExit(hold);
        exitTimer.interval = Math.max(1, hold + root.ms(140));
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
    }

    Indicator {
        id: indicator
        objectName: "indicatorPart"
        motionScale: root.motionScale
        easingType: root.easingType
    }
}
