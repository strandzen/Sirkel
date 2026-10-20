import QtQuick
import QtQuick.Effects
import QtQuick.Shapes

// Direction indicator: a static ring (square at cornerRadius 0, circle at
// cornerRadius >= width/2) with a pointer band that slides along the ring's
// own curve to the current pick, and wraps the whole ring for Maximize.
// Positioned by OverlayController::placeIndicator(); style properties are
// pushed from OverlayController::applyStyle().
Item {
    id: root
    transformOrigin: Item.Center
    visible: false

    // Placement name (placementName() in placement.cpp), its translated
    // label, and its position on the ring (0..7 clockwise from top, -1 for
    // none). ringIndex is set before direction.
    property string direction: "none"
    property string label: ""
    property int ringIndex: -1

    property real cornerRadius: 60
    property int ringWidth: 24
    property real pointerLength: 36
    property bool showText: false
    property color highlightColor: "#3daee9"
    property color ringColor: "#2e3440"
    // Ring opacity only; the pointer and text always stay at full strength.
    property real baseOpacity: 1.0
    // How committed the current pick reads, in [0.55, 1.0] (see
    // DirectionInput::intensity()). Dims the pointer for edges/corners.
    property real intensity: 1.0
    property real motionScale: 1
    property int easingType: Easing.OutBack
    // Bezier control points, used when easingType is BezierSpline.
    property var easingCurve: [0.22, 1, 0.36, 1, 1, 1]
    property bool shadowEnabled: true
    // Pointer shades from its color into a darker one.
    property bool twoTone: false

    // Soft drop shadow under the whole indicator.
    // The shadow needs a GPU renderer; with Qt Quick's software renderer
    // a layer effect makes the whole item vanish, so it's skipped there.
    layer.enabled: shadowEnabled && width > 0 && GraphicsInfo.api !== GraphicsInfo.Software
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: "black"
        shadowOpacity: 0.35
        shadowBlur: 0.6
        blurMax: 24
        shadowVerticalOffset: 2
    }

    function ms(base) {
        return Math.round(base * root.motionScale);
    }

    readonly property color markerFillColor: direction === "maximize"
        ? highlightColor
        : Qt.rgba(highlightColor.r * intensity, highlightColor.g * intensity, highlightColor.b * intensity, highlightColor.a)

    function reset() {
        entranceAnim.stop();
        exitAnim.stop();
        opacity = 1;
        scale = 1;
        visible = false;
        direction = "none";
        ringIndex = -1;
    }

    // Settles in: scales down from slightly larger while fading in, so it
    // reads as landing on the screen rather than popping up.
    function playEntrance() {
        visible = true;
        opacity = 0;
        scale = 1.25;
        entranceAnim.restart();
    }

    // Fades out while growing back towards 1.25x, the reverse of the
    // entrance, once `hold` ms (the outline's confirm flash) pass.
    function playExit(hold) {
        entranceAnim.stop();
        exitAnim.hold = hold;
        exitAnim.restart();
    }

    ParallelAnimation {
        id: entranceAnim
        NumberAnimation { target: root; property: "opacity"; to: 1; duration: root.ms(100); easing.type: Easing.OutCubic }
        NumberAnimation { target: root; property: "scale"; to: 1; duration: root.ms(100); easing.type: Easing.OutCubic }
    }

    SequentialAnimation {
        id: exitAnim
        property int hold: 0
        PauseAnimation { duration: exitAnim.hold }
        ParallelAnimation {
            NumberAnimation { target: root; property: "opacity"; to: 0; duration: root.ms(150); easing.type: Easing.InCubic }
            NumberAnimation { target: root; property: "scale"; to: 1.25; duration: root.ms(150); easing.type: Easing.InCubic }
        }
    }

    // Maximize "press-in": the whole ring shrinks a little while the center
    // is picked, like a button held down. A separate transform so it
    // composes with the entrance/exit animations on `scale`.
    property real pressScale: direction === "maximize" ? 0.85 : 1.0
    Behavior on pressScale {
        enabled: root.motionScale > 0
        NumberAnimation { duration: root.ms(150); easing.type: Easing.OutCubic }
    }
    transform: Scale {
        origin.x: root.width / 2
        origin.y: root.height / 2
        xScale: root.pressScale
        yScale: root.pressScale
    }

    // Ring geometry. The path runs along the ring's centerline, inset by
    // half the ring width; insetting a rounded corner shrinks its radius by
    // the same amount, hence rc rather than r for the arcs.
    readonly property real r: Math.min(cornerRadius, width / 2, height / 2)
    readonly property real inset: ringWidth / 2
    readonly property real rc: Math.max(0, r - inset)
    readonly property real straightX: Math.max(0, width - 2 * r)
    readonly property real straightY: Math.max(0, height - 2 * r)
    readonly property real arcLength: (Math.PI / 2) * rc
    readonly property real totalPathLength: 2 * straightX + 2 * straightY + 4 * arcLength
    // The loop clockwise from top-center, as [length, isArc] segments: half
    // the top edge, then corner/edge alternating, then the other half.
    readonly property var segments: [
        [straightX / 2, false], [arcLength, true], [straightY, false], [arcLength, true],
        [straightX, false], [arcLength, true], [straightY, false], [arcLength, true],
        [straightX / 2, false],
    ]

    // Point and tangent (degrees clockwise, like Item.rotation) at progress
    // t around the loop. t wraps. The 8 directions sit at exact 1/8 steps
    // by symmetry, whatever the corner radius.
    function pointAndAngleAt(tIn) {
        const t = ((tIn % 1) + 1) % 1;
        let rem = t * totalPathLength;
        const w = width, h = height;
        const L1 = straightX / 2, Lv = straightY, Lh = straightX, La = arcLength;
        const arcAt = (cx, cy, phi0) => {
            const phi = phi0 + (La > 0 ? rem / La : 0) * (Math.PI / 2);
            return { x: cx + rc * Math.cos(phi), y: cy + rc * Math.sin(phi), angle: (phi + Math.PI / 2) * 180 / Math.PI };
        };

        if (rem <= L1) return { x: w / 2 + rem, y: inset, angle: 0 };
        rem -= L1;
        if (rem <= La) return arcAt(w - r, r, -Math.PI / 2);
        rem -= La;
        if (rem <= Lv) return { x: w - inset, y: r + rem, angle: 90 };
        rem -= Lv;
        if (rem <= La) return arcAt(w - r, h - r, 0);
        rem -= La;
        if (rem <= Lh) return { x: (w - r) - rem, y: h - inset, angle: 180 };
        rem -= Lh;
        if (rem <= La) return arcAt(r, h - r, Math.PI / 2);
        rem -= La;
        if (rem <= Lv) return { x: inset, y: (h - r) - rem, angle: 270 };
        rem -= Lv;
        if (rem <= La) return arcAt(r, r, Math.PI);
        rem -= La;
        return { x: r + rem, y: inset, angle: 0 };
    }

    // Path lengths to sample between start and end: just the endpoints of
    // straight runs, and a point every ~3px around corners.
    function sampleLengths(start, end) {
        // Total derived from the same segments being walked: mid-resize,
        // the segments and totalPathLength bindings can briefly disagree,
        // and a total larger than the segments' sum never terminates.
        const segments = root.segments;
        const total = segments.reduce((sum, seg) => sum + seg[0], 0);
        if (!(total > 0) || !(end > start)) {
            return [];
        }
        let idx = 0;
        let segStart = Math.floor(start / total) * total;
        let seek = 0;
        while (segStart + segments[idx][0] <= start && seek++ < 64) {
            segStart += segments[idx][0];
            idx = (idx + 1) % segments.length;
        }
        const out = [start];
        let s = start;
        let guard = 0;
        while (s < end && guard++ < 64) {
            const segEnd = Math.min(segStart + segments[idx][0], end);
            if (segments[idx][1]) {
                const n = Math.max(1, Math.ceil((segEnd - s) / 3));
                for (let k = 1; k <= n; k++) {
                    out.push(s + (segEnd - s) * k / n);
                }
            } else {
                out.push(segEnd);
            }
            s = segEnd;
            segStart += segments[idx][0];
            idx = (idx + 1) % segments.length;
        }
        return out;
    }

    // Unwrapped so a slide always takes the shorter way round (topLeft ->
    // top goes forward through 0, not back through the bottom).
    property real unwrappedProgress: 0
    // Length of ring the pointer covers: pointerLength normally, the whole
    // loop for Maximize.
    property real effectiveSpan: pointerLength

    onRingIndexChanged: {
        // Maximize (-1) has no position of its own; the band stays put and
        // only grows.
        if (ringIndex < 0) {
            return;
        }
        const current = ((unwrappedProgress % 1) + 1) % 1;
        let delta = ringIndex / 8 - current;
        if (delta > 0.5) {
            delta -= 1;
        } else if (delta < -0.5) {
            delta += 1;
        }
        unwrappedProgress += delta;
    }
    onDirectionChanged: {
        if (direction !== "none") {
            effectiveSpan = direction === "maximize" ? totalPathLength : pointerLength;
        }
    }

    Behavior on unwrappedProgress {
        enabled: root.motionScale > 0
        NumberAnimation { duration: root.ms(260); easing.type: root.easingType; easing.bezierCurve: root.easingCurve }
    }
    Behavior on effectiveSpan {
        enabled: root.motionScale > 0
        NumberAnimation { duration: root.ms(75); easing.type: root.easingType; easing.bezierCurve: root.easingCurve }
    }

    // A band hugging the ring: each sample point on the centerline offset by
    // half the ring width to either side, with flat ends.
    readonly property var markerPolygon: {
        const center = (((unwrappedProgress % 1) + 1) % 1) * totalPathLength;
        const span = Math.min(effectiveSpan, totalPathLength);
        const lengths = sampleLengths(center - span / 2, center + span / 2);
        const total = Math.max(1, segments.reduce((sum, seg) => sum + seg[0], 0));
        const half = ringWidth / 2;
        const outer = [];
        const inner = [];
        for (const len of lengths) {
            const p = pointAndAngleAt(len / total);
            const rad = p.angle * Math.PI / 180;
            const nx = -Math.sin(rad), ny = Math.cos(rad);
            outer.push(Qt.point(p.x - nx * half, p.y - ny * half));
            inner.push(Qt.point(p.x + nx * half, p.y + ny * half));
        }
        inner.reverse();
        return outer.concat(inner);
    }

    Rectangle {
        anchors.fill: parent
        radius: root.r
        color: "transparent"
        border.width: root.ringWidth
        border.color: root.ringColor
        opacity: root.baseOpacity

        // Hairlines along the ring's outer and inner edge: a thin light
        // line that keeps the ring crisp against any background.
        Rectangle {
            anchors.fill: parent
            radius: root.r
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.18)
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: root.ringWidth - 1
            radius: Math.max(0, root.r - root.ringWidth + 1)
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.18)
        }
    }

    Shape {
        id: markerShape
        anchors.fill: parent
        visible: root.direction !== "none"
        // Analytic antialiasing; the default renderer leaves jagged edges.
        preferredRendererType: Shape.CurveRenderer

        property LinearGradient twoToneGradient: LinearGradient {
            x1: 0
            y1: 0
            x2: root.width
            y2: root.height
            GradientStop { position: 0; color: root.markerFillColor }
            GradientStop { position: 1; color: Qt.rgba(root.markerFillColor.r * 0.55, root.markerFillColor.g * 0.55, root.markerFillColor.b * 0.55, root.markerFillColor.a) }
        }

        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillColor: root.markerFillColor
            fillGradient: root.twoTone ? markerShape.twoToneGradient : null
            PathPolyline { path: root.markerPolygon }
        }
    }

    // System font, scaled with the indicator (16px at the default size of
    // 120). HorizontalFit shrinks a label that would cross the ring.
    Text {
        anchors.centerIn: parent
        width: Math.max(0, root.width - 2 * root.ringWidth - 8)
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 0.9
        color: "white"
        font.family: Qt.application.font.family
        font.pixelSize: Math.max(8, Math.round(root.width * 16 / 120))
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: 8
        text: root.label
        visible: root.showText && root.direction !== "none"
    }
}
