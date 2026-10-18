import QtQuick
import QtQuick.Shapes

// Center direction indicator: a static ring (square frame at cornerRadius 0,
// full circle at cornerRadius >= width/2 — see
// SirkelConfig::IndicatorCornerRadius) with a single marker that slides
// *along the ring's own boundary curve* to the currently selected
// direction — not a straight-line (chord-cutting) tween between two points,
// and not rotating around the center. Sized to the ring's thickness so it
// always sits pixel-fit inside it (Todo.md's "Other Notes" #8). Positioned
// as an explicit sub-rect (x/y/width/height, plain Item properties — no
// anchors.fill here deliberately) within the shared Overlay.qml window, set
// from SirkelEffect::repositionIndicator(); stacked above Outline by being
// declared after it there. All style properties are pushed from
// SirkelEffect::applyIndicatorStyle().
Item {
    id: root
    transformOrigin: Item.Center

    property string direction: "none"
    property real cornerRadius: 110
    property int ringWidth: 12
    property real pointerLength: ringWidth * 3
    property bool showText: true
    // highlightColor is the pointer/marker's own color (SirkelConfig::
    // IndicatorPointerColor et al) — ringColor (SirkelConfig::
    // IndicatorRingColor et al) is the static base ring's, entirely
    // independent unless LinkIndicatorColors is on. Full alpha by default,
    // unlike the ring's old hardcoded color which baked a fixed ~40% alpha
    // directly in - transparency is baseOpacity's job now, not the color's.
    property color highlightColor: "#3daee9"
    property color ringColor: "#2e3440"
    // Shared with Outline.qml's own glide/resize animation (both are driven
    // by SirkelConfig::OutlineAnimationDuration) — 0 means every change
    // below is instant, matching the original behavior.
    property int animationDuration: 0
    // "linear" | "smooth" | "overshoot" — see Easing's kcfg doc comment.
    // Shared with Outline.qml (both read the same setting).
    property string easing: "overshoot"

    function easingType() {
        switch (root.easing) {
        case "linear": return Easing.Linear;
        case "smooth": return Easing.OutCubic;
        default: return Easing.OutBack;
        }
    }

    // SirkelConfig::IndicatorRingFillOpacity / 100 — applies only to the
    // static base ring below, never to the pointer or text, which always
    // stay at full strength regardless. (A ring-blur option was tried here
    // too and pulled back out — see Todo.md's Unscoped ideas for why.)
    property real baseOpacity: 1.0
    // Pushed from SirkelEffect::updateDirectionIntensity() on every
    // direction update, already mapped to [0.55, 1.0] — how "committed"
    // the current drag reads as. Used by markerFillColor below for every
    // direction except Maximize, which has its own feedback instead
    // (breatheOpacity).
    property real intensity: 1.0

    function dimmedColor(c, factor) {
        return Qt.rgba(c.r * factor, c.g * factor, c.b * factor, c.a);
    }

    readonly property color markerFillColor: (direction === "maximize" || direction === "none")
        ? highlightColor
        : dimmedColor(highlightColor, intensity)

    // Slow opacity "breathing" while resting in the Maximize zone — a quiet
    // "hold here to confirm" cue, distinct from the brightness-based
    // feedback edges/corners get from `intensity`. Only ever loops while
    // actually in that zone during an armed session.
    property real breatheOpacity: 1.0

    SequentialAnimation {
        id: breatheAnim
        loops: Animation.Infinite
        running: root.direction === "maximize"
        onRunningChanged: if (!running) root.breatheOpacity = 1.0
        NumberAnimation { target: root; property: "breatheOpacity"; to: 0.72; duration: 650; easing.type: Easing.InOutSine }
        NumberAnimation { target: root; property: "breatheOpacity"; to: 1.0; duration: 650; easing.type: Easing.InOutSine }
    }

    // Called once from SirkelEffect::engage() the moment real input first
    // arrives each session — pops the whole indicator in (scale + fade
    // from ~88%) instead of snapping straight to fully drawn. Explicitly
    // invoked rather than keyed off `visible` changing: root.visible only
    // ever transitions false->true *once* in the plugin's entire lifetime
    // in practice (nothing ever sets it back to false between sessions),
    // not once per session as an onVisibleChanged-driven version assumed —
    // that version only ever played on the very first snap after KWin
    // loaded the effect.
    function playEntrance() {
        if (root.animationDuration <= 0) {
            opacity = 1;
            return;
        }
        opacity = 0;
        scale = 0.88;
        entranceAnim.restart();
    }

    // Targets a flat 1, not baseOpacity — this animates the whole item
    // (pointer and text included) popping in together; baseOpacity is a
    // separate, ring-only multiplier applied below on baseRing, and
    // composes correctly through it either way since child opacity
    // multiplies with the parent's in Qt Quick.
    ParallelAnimation {
        id: entranceAnim
        NumberAnimation { target: root; property: "opacity"; to: 1; duration: 180; easing.type: Easing.OutCubic }
        NumberAnimation { target: root; property: "scale"; to: 1; duration: 180; easing.type: Easing.OutBack }
    }

    readonly property real r: Math.min(cornerRadius, width / 2, height / 2)
    readonly property real inset: ringWidth / 2
    // The border is drawn on the ring's centerline, inset from its outer
    // edge by `inset` — offsetting a rounded corner inward by a distance
    // shrinks its own radius by that same distance (standard rounded-rect
    // offset geometry), not the outer radius again. Using `r` for the arc
    // (as the first version of this did) gave the path's endpoints and its
    // radius two different corners' worth of geometry, so the actual curve
    // Qt drew for them came out smaller/flatter than the ring's real curve
    // — the "pulled toward center" look.
    readonly property real rc: Math.max(0, r - inset)

    // The 8 directions sit at exactly 1/8 intervals of the path's arc
    // length, regardless of cornerRadius: walking from one waypoint to the
    // next always covers (one straight edge's half) + (one corner arc's
    // half), and by symmetry every one of the 8 gaps has that same length.
    // So no per-direction arc-length bookkeeping is needed — just index/8.
    readonly property var directionOrder: ["top", "topRight", "right", "bottomRight", "bottom", "bottomLeft", "left", "topLeft"]

    function progressForDirection(dir) {
        const index = directionOrder.indexOf(dir);
        return index >= 0 ? index / 8 : 0;
    }

    readonly property var directionLabels: ({
        "top": "Top",
        "bottom": "Bottom",
        "left": "Left",
        "right": "Right",
        "topRight": "Top\nRight",
        "topLeft": "Top\nLeft",
        "bottomRight": "Bottom\nRight",
        "bottomLeft": "Bottom\nLeft",
        "maximize": "Maximize",
    })

    function labelForDirection(dir) {
        return directionLabels[dir] || "";
    }

    // Animated via unwrappedProgress rather than driving pathInterpolator's
    // progress directly, so the transition can always take the *shorter*
    // way around the ring (e.g. topLeft -> top goes forward through 0/1,
    // not backward through right/bottom/left) the same way
    // RotationAnimation.Shortest would for an angle — plain number
    // animation has no such wraparound awareness on its own.
    property real unwrappedProgress: 0

    // How much of the ring's curve the pointer band currently covers.
    // Normally just pointerLength, but grows out symmetrically in both
    // directions to the full loop (totalPathLength) for the "maximize"
    // gesture, and shrinks back down again on leaving it — see
    // onDirectionChanged and markerPolygon below.
    property real effectiveSpan: pointerLength

    onDirectionChanged: {
        // "maximize" has no edge/corner of its own to point at, so the
        // band's *position* freezes wherever it last was — only its span
        // grows to wrap the whole ring. Position resumes sliding normally
        // the next time a real direction is picked.
        if (direction !== "none" && direction !== "maximize") {
            const target = progressForDirection(direction);
            const current = ((unwrappedProgress % 1) + 1) % 1;
            let delta = target - current;
            if (delta > 0.5) {
                delta -= 1;
            } else if (delta < -0.5) {
                delta += 1;
            }
            unwrappedProgress += delta;
        }
        if (direction !== "none") {
            effectiveSpan = (direction === "maximize") ? totalPathLength : pointerLength;
        }
    }

    Behavior on unwrappedProgress {
        NumberAnimation { duration: 260; easing.type: root.easingType() }
    }

    Behavior on effectiveSpan {
        enabled: root.animationDuration > 0
        NumberAnimation { duration: root.animationDuration; easing.type: root.easingType() }
    }

    // Static base ring — same shape always, never swaps pieces. color/
    // opacity below are the only things IndicatorRingColor/
    // IndicatorRingFillOpacity ever touch - the pointer (Shape, below) and
    // text always stay at full strength regardless.
    Rectangle {
        id: baseRing
        anchors.fill: parent
        radius: root.r
        color: "transparent"
        border.width: root.ringWidth
        border.color: root.ringColor
        opacity: root.baseOpacity
    }

    // The ring's boundary centerline has the same shape QML's Path/PathArc
    // would draw (traced clockwise from top-center: a straight run, a
    // quarter-circle corner, a straight run, ...), reproduced here as plain
    // arc-length math instead of a declarative Path so the marker below can
    // sample *many* points along it (a single PathInterpolator only ever
    // exposes one point at a time). Returns the point and the tangent
    // direction (in the same degrees-clockwise convention as Item.rotation)
    // at a given progress t around the loop (t wraps, like the ring itself).
    readonly property real totalPathLength: {
        const straightX = Math.max(0, width - 2 * r);
        const straightY = Math.max(0, height - 2 * r);
        return 2 * straightX + 2 * straightY + 2 * Math.PI * rc;
    }

    function pointAndAngleAt(tIn) {
        const t = ((tIn % 1) + 1) % 1;
        let rem = t * root.totalPathLength;
        const L1 = Math.max(0, root.width / 2 - root.r);
        const Lv = Math.max(0, root.height - 2 * root.r);
        const Lh = Math.max(0, root.width - 2 * root.r);
        const Larc = (Math.PI / 2) * root.rc;

        // top straight, first half — from (width/2, inset) going +x
        if (rem <= L1) {
            return { x: root.width / 2 + rem, y: root.inset, angle: 0 };
        }
        rem -= L1;
        // top-right corner arc, center (width-r, r), phi -90 -> 0
        if (rem <= Larc) {
            const phi = -Math.PI / 2 + (Larc > 0 ? rem / Larc : 0) * (Math.PI / 2);
            return { x: (root.width - root.r) + root.rc * Math.cos(phi), y: root.r + root.rc * Math.sin(phi), angle: (phi + Math.PI / 2) * 180 / Math.PI };
        }
        rem -= Larc;
        // right straight — from (width-inset, r) going +y
        if (rem <= Lv) {
            return { x: root.width - root.inset, y: root.r + rem, angle: 90 };
        }
        rem -= Lv;
        // bottom-right corner arc, center (width-r, height-r), phi 0 -> 90
        if (rem <= Larc) {
            const phi = (Larc > 0 ? rem / Larc : 0) * (Math.PI / 2);
            return { x: (root.width - root.r) + root.rc * Math.cos(phi), y: (root.height - root.r) + root.rc * Math.sin(phi), angle: (phi + Math.PI / 2) * 180 / Math.PI };
        }
        rem -= Larc;
        // bottom straight — from (width-r, height-inset) going -x
        if (rem <= Lh) {
            return { x: (root.width - root.r) - rem, y: root.height - root.inset, angle: 180 };
        }
        rem -= Lh;
        // bottom-left corner arc, center (r, height-r), phi 90 -> 180
        if (rem <= Larc) {
            const phi = Math.PI / 2 + (Larc > 0 ? rem / Larc : 0) * (Math.PI / 2);
            return { x: root.r + root.rc * Math.cos(phi), y: (root.height - root.r) + root.rc * Math.sin(phi), angle: (phi + Math.PI / 2) * 180 / Math.PI };
        }
        rem -= Larc;
        // left straight — from (inset, height-r) going -y
        if (rem <= Lv) {
            return { x: root.inset, y: (root.height - root.r) - rem, angle: 270 };
        }
        rem -= Lv;
        // top-left corner arc, center (r, r), phi 180 -> 270
        if (rem <= Larc) {
            const phi = Math.PI + (Larc > 0 ? rem / Larc : 0) * (Math.PI / 2);
            return { x: root.r + root.rc * Math.cos(phi), y: root.r + root.rc * Math.sin(phi), angle: (phi + Math.PI / 2) * 180 / Math.PI };
        }
        rem -= Larc;
        // top straight, second half — from (r, inset) going +x, back to start
        return { x: root.r + rem, y: root.inset, angle: 0 };
    }

    // The sliding marker's fill shape: a band that hugs the ring's own curve
    // (built by sampling several points along the boundary around the
    // current position and offsetting each one by half the ring's own
    // thickness to either side — the pointer always fills the ring it rides
    // on widthwise, only its length varies) — this is what gives it a curved
    // edge at a corner instead of being a straight bar laid across the
    // curve. Its two short ends are a straight cut connecting the outer and
    // inner offset of the same sample point, so they come out flat/square
    // rather than rounded. Loop's own indicator marker works the same way: a
    // thick segment that bends with the ring. effectiveSpan growing all the
    // way to totalPathLength (the "maximize" gesture) makes the two ends
    // meet at the same point on the ring, so this doubles as the full-ring
    // fill with no separate shape needed — just a lot more of the same band.
    readonly property var markerPolygon: {
        // A fixed sample count looked smooth for a short band but visibly
        // faceted once effectiveSpan grows to the full ring (the same 41
        // points stretched around four corners instead of concentrated at
        // one) — sample at a roughly constant spacing along the curve
        // instead, so corners stay smooth at any span.
        const samples = Math.max(11, Math.min(300, Math.ceil(effectiveSpan / 3) + 1));
        const half = ringWidth / 2;
        const halfSpanT = Math.min(0.5, (effectiveSpan / 2) / Math.max(1, totalPathLength));
        const center = ((unwrappedProgress % 1) + 1) % 1;
        let outer = [];
        let inner = [];
        for (let i = 0; i < samples; i++) {
            const frac = (i / (samples - 1)) * 2 - 1;
            const p = pointAndAngleAt(center + frac * halfSpanT);
            const rad = p.angle * Math.PI / 180;
            const ndx = -Math.sin(rad), ndy = Math.cos(rad);
            outer.push(Qt.point(p.x - ndx * half, p.y - ndy * half));
            inner.push(Qt.point(p.x + ndx * half, p.y + ndy * half));
        }
        inner.reverse();
        return outer.concat(inner);
    }

    Shape {
        anchors.fill: parent
        visible: root.direction !== "none"
        opacity: root.direction === "maximize" ? root.breatheOpacity : 1.0
        ShapePath {
            strokeWidth: 0
            fillColor: root.markerFillColor
            PathPolyline { path: root.markerPolygon }
        }
    }

    // System font (whatever KDE's General font is set to), sized relative to
    // the indicator's own width — 16px at the default IndicatorSize of 120,
    // same as the old fixed size. HorizontalFit then shrinks long labels so
    // they never spill past the ring's inner edge.
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
        text: root.labelForDirection(root.direction)
        visible: root.showText && root.direction !== "none"
    }
}
