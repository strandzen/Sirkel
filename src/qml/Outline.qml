import QtQuick
import QtQuick.Effects

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
    property real motionScale: 1
    property int easingType: Easing.OutBack
    // Bezier control points, used when easingType is BezierSpline.
    property var easingCurve: [0.22, 1, 0.47, 1, 1, 1]
    property bool shadowEnabled: true
    // Fill shades from borderColor into secondColor.
    property bool twoTone: false
    readonly property color secondColor: Qt.rgba(borderColor.r * 0.55, borderColor.g * 0.55, borderColor.b * 0.55, borderColor.a)

    property real targetX: 0
    property real targetY: 0
    property real targetWidth: 0
    property real targetHeight: 0

    // The rect actually drawn right now, mid-animation included. The blur
    // region follows this.
    readonly property rect visualRect: Qt.rect(targetX, targetY, Math.max(0, targetWidth), Math.max(0, targetHeight))

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
        exitAnim.stop();
        opacity = 1;
        visible = false;
    }

    function playEntrance() {
        visible = true;
        fillRect.opacity = 0;
        entranceAnim.restart();
    }

    function playExit() {
        entranceAnim.stop();
        exitAnim.restart();
    }

    // easing.overshoot tones OutBack down: the default bounce is too much on
    // a whole filled rect.
    component Glide: NumberAnimation {
        duration: root.ms(75)
        easing.type: root.easingType
        easing.bezierCurve: root.easingCurve
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

    NumberAnimation {
        id: exitAnim
        target: root
        property: "opacity"
        to: 0
        duration: root.ms(140)
        easing.type: Easing.InCubic
    }

    // Two-tone: the fill shades from borderColor into secondColor, top to
    // bottom. Several stops instead of two: a 2-stop ramp bands at low alpha.
    function gradientStop(position) {
        const c = Qt.tint(borderColor, Qt.rgba(secondColor.r, secondColor.g, secondColor.b, position));
        return Qt.rgba(c.r, c.g, c.b, fillOpacity / 100);
    }

    Gradient {
        id: fillGradient
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
        gradient: root.twoTone ? fillGradient : null
        border.color: root.borderColor
        border.width: root.borderWidth
        radius: root.cornerRadius

        // Soft drop shadow (Loop uses about black 20%, radius 10).
        // Skipped on the software renderer, where it would hide the item.
        layer.enabled: root.shadowEnabled && width > 0 && height > 0 && GraphicsInfo.api !== GraphicsInfo.Software
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "black"
            shadowOpacity: 0.35
            shadowBlur: 0.6
            blurMax: 24
            shadowVerticalOffset: 2
        }
    }
}
