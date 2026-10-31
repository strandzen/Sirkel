// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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

    // Placement name (placementName() in placement.cpp) and its position on
    // the ring (0..7 clockwise from top, -1 for none). ringIndex is set
    // before direction.
    property string direction: "none"
    property int ringIndex: -1

    property real cornerRadius: 60
    property int ringWidth: 24
    // The thickness actually drawn: at most half the indicator, or the
    // ring would fill it and the pointer band fold over itself.
    readonly property real band: Math.max(0, Math.min(ringWidth, width / 2, height / 2))
    // Length of the pointer along the ring: follows the indicator's size
    // (36 px at the default 120 px).
    readonly property real pointerLength: width * 0.3
    property color highlightColor: "#3daee9"
    property color ringColor: "#2e3440"
    // Ring opacity only; the pointer always stays at full strength.
    property real baseOpacity: 1.0
    // How committed the current pick reads, in [0.55, 1.0] (see
    // DirectionInput::intensity()). Dims the pointer for edges/corners.
    property real intensity: 1.0
    property real motionScale: 1
    property int easingType: Easing.OutBack
    // Bezier control points, used when easingType is BezierSpline.
    property var easingCurve: [0.22, 1, 0.36, 1, 1, 1]

    // Soft drop shadow under the whole indicator.
    // The shadow needs a GPU renderer; with Qt Quick's software renderer
    // a layer effect makes the whole item vanish, so it's skipped there.
    layer.enabled: width > 0 && GraphicsInfo.api !== GraphicsInfo.Software
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
        // The first pick lands directly, rather than sliding over from
        // wherever the previous session left the pointer.
        jumpNext = true;
        lastRingIndex = -1;
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
    // entrance.
    function playExit() {
        entranceAnim.stop();
        exitAnim.restart();
    }

    ParallelAnimation {
        id: entranceAnim
        NumberAnimation { target: root; property: "opacity"; to: 1; duration: root.ms(100); easing.type: Easing.OutCubic }
        NumberAnimation { target: root; property: "scale"; to: 1; duration: root.ms(100); easing.type: Easing.OutCubic }
    }

    ParallelAnimation {
        id: exitAnim
        NumberAnimation { target: root; property: "opacity"; to: 0; duration: root.ms(150); easing.type: Easing.InCubic }
        NumberAnimation { target: root; property: "scale"; to: 1.25; duration: root.ms(150); easing.type: Easing.InCubic }
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
    // the same amount, hence rc rather than r for the arcs. Corners rounder
    // than the inset are centered `cr` in from the edges; sharper ones meet
    // in a point at the inset (cr = inset, rc = 0).
    readonly property real r: Math.min(cornerRadius, width / 2, height / 2)
    readonly property real inset: band / 2
    readonly property real cr: Math.max(r, inset)
    readonly property real rc: cr - inset
    // The hole's corner radius: the outer one scaled to the hole's size, so
    // inner and outer corners round together (a circle stays a circle).
    // Thicker at the corners than a constant-width ring, which would keep
    // the inside square until the roundness passes the ring width.
    readonly property real holeSize: Math.max(0, Math.min(width, height) - 2 * band)
    readonly property real ri: Math.min(holeSize / 2, Math.min(width, height) > 0 ? r * holeSize / Math.min(width, height) : 0)
    readonly property real straightX: Math.max(0, width - 2 * cr)
    readonly property real straightY: Math.max(0, height - 2 * cr)
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
        const c = cr;
        if (rem <= La) return arcAt(w - c, c, -Math.PI / 2);
        rem -= La;
        if (rem <= Lv) return { x: w - inset, y: c + rem, angle: 90 };
        rem -= Lv;
        if (rem <= La) return arcAt(w - c, h - c, 0);
        rem -= La;
        if (rem <= Lh) return { x: (w - c) - rem, y: h - inset, angle: 180 };
        rem -= Lh;
        if (rem <= La) return arcAt(c, h - c, Math.PI / 2);
        rem -= La;
        if (rem <= Lv) return { x: inset, y: (h - c) - rem, angle: 270 };
        rem -= Lv;
        if (rem <= La) return arcAt(c, c, Math.PI);
        rem -= La;
        return { x: c + rem, y: inset, angle: 0 };
    }

    // Path lengths to sample between start and end: a point every ~3px.
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
            // Every ~3px, straights too: the hole's rounded corners reach
            // into the straight runs (see holePoint()).
            const n = Math.max(1, Math.ceil((segEnd - s) / 3));
            for (let k = 1; k <= n; k++) {
                out.push(s + (segEnd - s) * k / n);
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
    // Set by reset(): the session's first pick skips the slide and grow
    // animations. Cleared once that pick's direction is in.
    property bool jumpNext: true
    // The last real ring position (Maximize leaves it as it was), and set
    // while jumping to the opposite side (see onRingIndexChanged).
    property int lastRingIndex: -1
    property bool jumpOpposite: false

    onRingIndexChanged: {
        // Maximize (-1) has no position of its own; the band stays put and
        // only grows.
        if (ringIndex < 0) {
            return;
        }
        // Directly opposite (Left -> Right): both ways round are equally
        // long, so a slide would sweep half the ring through unrelated
        // directions. Jump instead, like Loop.
        const opposite = lastRingIndex >= 0 && (ringIndex - lastRingIndex + 8) % 8 === 4;
        lastRingIndex = ringIndex;
        const current = ((unwrappedProgress % 1) + 1) % 1;
        let delta = ringIndex / 8 - current;
        if (delta > 0.5) {
            delta -= 1;
        } else if (delta < -0.5) {
            delta += 1;
        }
        jumpOpposite = opposite;
        unwrappedProgress += delta;
        jumpOpposite = false;
    }
    onDirectionChanged: {
        if (direction !== "none") {
            effectiveSpan = direction === "maximize" ? totalPathLength : pointerLength;
            jumpNext = false;
        }
    }

    Behavior on unwrappedProgress {
        enabled: root.motionScale > 0 && !root.jumpNext && !root.jumpOpposite
        NumberAnimation { duration: root.ms(260); easing.type: root.easingType; easing.bezierCurve: root.easingCurve }
    }
    Behavior on effectiveSpan {
        enabled: root.motionScale > 0 && !root.jumpNext
        NumberAnimation { duration: root.ms(75); easing.type: root.easingType; easing.bezierCurve: root.easingCurve }
    }

    // A band hugging the ring: each sample point on the centerline offset by
    // half the ring width to either side, with flat ends.
    // The closest point on the hole's edge (its rounded corners included):
    // where the pointer's inner side goes, so it covers the ring exactly.
    function holePoint(x, y) {
        const x0 = band, y0 = band, x1 = width - band, y1 = height - band, q = ri;
        const cx = Math.min(Math.max(x, x0 + q), x1 - q);
        const cy = Math.min(Math.max(y, y0 + q), y1 - q);
        if (cx !== x && cy !== y) {
            const dx = x - cx, dy = y - cy, d = Math.hypot(dx, dy);
            return d > 0 ? Qt.point(cx + dx / d * q, cy + dy / d * q) : Qt.point(cx, cy);
        }
        const dl = Math.abs(x - x0), dr = Math.abs(x1 - x), dt = Math.abs(y - y0), db = Math.abs(y1 - y);
        const m = Math.min(dl, dr, dt, db);
        if (m === dt) return Qt.point(Math.min(Math.max(x, x0), x1), y0);
        if (m === db) return Qt.point(Math.min(Math.max(x, x0), x1), y1);
        if (m === dl) return Qt.point(x0, Math.min(Math.max(y, y0), y1));
        return Qt.point(x1, Math.min(Math.max(y, y0), y1));
    }

    readonly property var markerPolygon: {
        const center = (((unwrappedProgress % 1) + 1) % 1) * totalPathLength;
        const span = Math.min(effectiveSpan, totalPathLength);
        const lengths = sampleLengths(center - span / 2, center + span / 2);
        const total = Math.max(1, segments.reduce((sum, seg) => sum + seg[0], 0));
        const half = band / 2;
        const outer = [];
        const inner = [];
        let prev = null;
        for (const len of lengths) {
            const p = pointAndAngleAt(len / total);
            const rad = p.angle * Math.PI / 180;
            const nx = -Math.sin(rad), ny = Math.cos(rad);
            // A sharp (or barely rounded) corner turns 90 degrees between
            // two samples: add the corner itself, mitered, or the band
            // would cut across it.
            const turn = ((p.angle - (prev ? prev.angle : p.angle)) % 360 + 360) % 360;
            if (prev && turn > 45 && turn < 135) {
                const prad = prev.angle * Math.PI / 180;
                const dx1 = Math.cos(prad), dy1 = Math.sin(prad);
                const dx2 = Math.cos(rad), dy2 = Math.sin(rad);
                const cross = dx1 * dy2 - dy1 * dx2;
                const t = ((p.x - prev.x) * dy2 - (p.y - prev.y) * dx2) / cross;
                const cx = prev.x + t * dx1, cy = prev.y + t * dy1;
                const mx = -Math.sin(prad) + nx, my = Math.cos(prad) + ny;
                // The ring's outer corner is still rounded by r (smaller
                // than the inset here): follow it rather than poke out.
                const ox = cx - mx * half, oy = cy - my * half;
                const ar = Math.min(root.r, half);
                const ax = ox + mx * ar, ay = oy + my * ar;
                const a1 = Math.atan2(Math.cos(prad), -Math.sin(prad)) + Math.PI;
                const steps = ar > 0.5 ? 6 : 0;
                for (let k = 0; k <= steps; k++) {
                    const a = a1 + (Math.PI / 2) * (steps > 0 ? k / steps : 0.5);
                    outer.push(Qt.point(steps > 0 ? ax + ar * Math.cos(a) : ox, steps > 0 ? ay + ar * Math.sin(a) : oy));
                }
                inner.push(holePoint(cx + mx * half, cy + my * half));
            }
            outer.push(Qt.point(p.x - nx * half, p.y - ny * half));
            inner.push(holePoint(p.x + nx * half, p.y + ny * half));
            prev = p;
        }
        inner.reverse();
        return outer.concat(inner);
    }

    Item {
        anchors.fill: parent
        opacity: root.baseOpacity

        // The ring: the outer rounded rectangle minus the hole.
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeWidth: 0
                strokeColor: "transparent"
                fillColor: root.ringColor
                fillRule: ShapePath.OddEvenFill
                PathRectangle {
                    width: root.width
                    height: root.height
                    radius: root.r
                }
                PathRectangle {
                    x: root.band
                    y: root.band
                    width: root.width - 2 * root.band
                    height: root.height - 2 * root.band
                    radius: root.ri
                }
            }
        }

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
            anchors.margins: root.band - 1
            radius: root.ri > 0 ? root.ri + 1 : 0
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

        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillColor: root.markerFillColor
            PathPolyline { path: root.markerPolygon }
        }
    }
}
