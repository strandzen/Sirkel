import QtQuick

// Preview of where the window will land. Fills the overlay window; only the
// inner rect (targetX/Y/Width/Height, window-local) moves, so it can glide
// between picks with Behavior animations. Style properties are pushed from
// OverlayController::applyStyle().
Item {
    id: root
    visible: false

    property real borderWidth: 3
    property color borderColor: "#3daee9"
    property real cornerRadius: 6
    // 0-100.
    property real fillOpacity: 0
    property bool confirmFlashEnabled: true
    property bool gradientFillEnabled: false
    property real motionScale: 1
    property int easingType: Easing.OutBack

    // The screen edge the current placement pulls against: -1 left/top,
    // 1 right/bottom, 0 none. Orients the gradient fill.
    property int edgeH: 0
    property int edgeV: 0

    property real targetX: 0
    property real targetY: 0
    property real targetWidth: 0
    property real targetHeight: 0

    // Extra size during the confirm pop, in px on every side.
    property real popGrow: 0

    // The rect actually drawn right now, mid-animation included. The blur
    // region follows this.
    readonly property rect visualRect: Qt.rect(targetX - popGrow, targetY - popGrow, Math.max(0, targetWidth + 2 * popGrow), Math.max(0, targetHeight + 2 * popGrow))

    // Set while jumping without animation (see jumpTo()).
    property bool instant: false

    function ms(base) {
        return Math.round(base * root.motionScale);
    }

    // Behaviors are disabled during the assignment, so the next target
    // change animates from here.
    function jumpTo(x, y, w, h) {
        instant = true;
        targetX = x;
        targetY = y;
        targetWidth = w;
        targetHeight = h;
        instant = false;
    }

    function reset() {
        entranceAnim.stop();
        confirmFlashAnim.stop();
        confirmPopAnim.stop();
        exitAnim.stop();
        popGrow = 0;
        flashOverlay.opacity = 0;
        opacity = 1;
        visible = false;
    }

    function playEntrance() {
        visible = true;
        fillRect.opacity = 0;
        entranceAnim.restart();
    }

    // Fades out after the confirm flash (if any). Returns how long the
    // flash holds before the fade starts, so the indicator can match.
    function playExit(confirmed) {
        entranceAnim.stop();
        let hold = 0;
        if (confirmed && confirmFlashEnabled && visible) {
            confirmFlashAnim.restart();
            confirmPopAnim.restart();
            hold = ms(250);
        }
        exitAnim.hold = hold;
        exitAnim.restart();
        return hold;
    }

    // easing.overshoot tones OutBack down: the default bounce is too much on
    // a whole filled rect.
    component Glide: NumberAnimation {
        duration: root.ms(75)
        easing.type: root.easingType
        easing.overshoot: 0.6
    }
    Behavior on targetX { enabled: root.motionScale > 0 && !root.instant; Glide {} }
    Behavior on targetY { enabled: root.motionScale > 0 && !root.instant; Glide {} }
    Behavior on targetWidth { enabled: root.motionScale > 0 && !root.instant; Glide {} }
    Behavior on targetHeight { enabled: root.motionScale > 0 && !root.instant; Glide {} }

    NumberAnimation {
        id: entranceAnim
        target: fillRect
        property: "opacity"
        to: 1
        duration: root.ms(180)
        easing.type: Easing.OutCubic
    }

    SequentialAnimation {
        id: confirmPopAnim
        NumberAnimation { target: root; property: "popGrow"; to: 6; duration: root.ms(80); easing.type: Easing.OutQuad }
        NumberAnimation { target: root; property: "popGrow"; to: 0; duration: root.ms(180); easing.type: Easing.OutBack }
    }

    SequentialAnimation {
        id: confirmFlashAnim
        NumberAnimation { target: flashOverlay; property: "opacity"; to: 0.85; duration: root.ms(90); easing.type: Easing.OutQuad }
        NumberAnimation { target: flashOverlay; property: "opacity"; to: 0; duration: root.ms(320); easing.type: Easing.InQuad }
    }

    SequentialAnimation {
        id: exitAnim
        property int hold: 0
        PauseAnimation { duration: exitAnim.hold }
        NumberAnimation { target: root; property: "opacity"; to: 0; duration: root.ms(140); easing.type: Easing.InCubic }
    }

    // Alpha envelope along the gradient's solid->faint axis (0..1). Several
    // stops instead of two: a linear 2-stop ramp bands at low alpha.
    function gradientAlpha(pos) {
        return Math.min(1, root.fillOpacity / 100 * (0.4 + (1 - pos * pos) * 1.2));
    }
    readonly property bool gradientHorizontal: edgeH !== 0
    readonly property bool gradientSolidAtStart: gradientHorizontal ? edgeH < 0 : edgeV <= 0
    function gradientStop(position) {
        const pos = gradientSolidAtStart ? position : 1 - position;
        return Qt.rgba(borderColor.r, borderColor.g, borderColor.b, gradientAlpha(pos));
    }

    Gradient {
        id: fillGradient
        orientation: root.gradientHorizontal ? Gradient.Horizontal : Gradient.Vertical
        GradientStop { position: 0.00; color: root.gradientStop(0.00) }
        GradientStop { position: 0.25; color: root.gradientStop(0.25) }
        GradientStop { position: 0.50; color: root.gradientStop(0.50) }
        GradientStop { position: 0.75; color: root.gradientStop(0.75) }
        GradientStop { position: 1.00; color: root.gradientStop(1.00) }
    }

    Rectangle {
        id: fillRect
        x: root.visualRect.x + root.borderWidth / 2
        y: root.visualRect.y + root.borderWidth / 2
        width: Math.max(0, root.visualRect.width - root.borderWidth)
        height: Math.max(0, root.visualRect.height - root.borderWidth)
        color: Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, root.fillOpacity / 100)
        gradient: root.gradientFillEnabled ? fillGradient : null
        border.color: root.borderColor
        border.width: root.borderWidth
        radius: root.cornerRadius

        // The confirm flash's brightness pulse.
        Rectangle {
            id: flashOverlay
            anchors.fill: parent
            radius: parent.radius
            color: root.borderColor
            opacity: 0
        }
    }
}
