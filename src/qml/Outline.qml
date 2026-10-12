import QtQuick

// Preview of where the window will land. Fills the shared Overlay.qml
// window (anchors.fill: parent, same as always — that window stays fixed
// for the whole session, sized/positioned to cover the target screen's
// entire usable area, set once by SirkelEffect::arm() via m_outlineArea)
// — only the inner Rectangle's geometry changes as the direction changes,
// in window-local coordinates (targetX/Y/Width/Height, pushed by
// SirkelEffect::setDirection()). That's what lets it glide/grow between
// directions via ordinary QML Behavior animations instead of an instant
// window move+resize, which can't be animated the same way. Declared
// before Indicator in Overlay.qml, so Indicator always paints on top of
// this, not the other way around — see Overlay.qml. Only the border (and,
// optionally, a translucent fill inside it) is visible — the live desktop
// shows through everywhere outside it, and through the inside too when
// fillOpacity is 0. borderWidth, borderColor, cornerRadius, fillOpacity,
// and animationDuration are pushed from
// SirkelEffect::applyIndicatorStyle() (SirkelConfig::OutlineBorderWidth /
// OutlineColor / OutlineCornerRadius / OutlineFillOpacity /
// OutlineAnimationDuration). easing, confirmFlashEnabled, and
// gradientFillEnabled are pushed from the same place (SirkelConfig::Easing
// / OutlineConfirmFlash / OutlineGradientFill) — see each kcfg entry's own
// doc comment for what it does.
Item {
    id: root
    anchors.fill: parent

    property real borderWidth: 3
    property color borderColor: "#3daee9"
    property real cornerRadius: 6
    property real fillOpacity: 0
    property int animationDuration: 0
    // "linear" | "smooth" | "overshoot" — see Easing's kcfg doc comment.
    property string easing: "overshoot"
    property bool confirmFlashEnabled: true
    property bool gradientFillEnabled: false

    function easingType() {
        switch (root.easing) {
        case "linear": return Easing.Linear;
        case "smooth": return Easing.OutCubic;
        default: return Easing.OutBack;
        }
    }
    // Pushed from SirkelEffect::setDirection(), same string values as
    // Indicator.qml's own `direction` (snapDirectionName()) — used only to
    // orient the gradient fill toward whichever screen edge the pick is
    // pulling the window against (see fillGradient below).
    property string direction: "none"

    // The gradient reads as "pulled toward the edge this direction snaps
    // against": solid near that edge, fading out toward the opposite side
    // — e.g. Left is solid on its left, fading out to the right, and Right
    // is the mirror of that. Corners reuse their own horizontal edge (they
    // already only scroll-resize horizontally, see snapTargetGeometry), so
    // TopLeft/BottomLeft read the same as Left and TopRight/BottomRight
    // read the same as Right. Maximize/none have no edge to pull toward,
    // so they keep the original top-brighter/bottom-dimmer default.
    readonly property bool gradientHorizontal: direction === "left" || direction === "right"
        || direction === "topLeft" || direction === "topRight"
        || direction === "bottomLeft" || direction === "bottomRight"
    readonly property bool gradientSolidAtStart: direction === "left" || direction === "top"
        || direction === "topLeft" || direction === "bottomLeft"
        || direction === "maximize" || direction === "none"

    // Window-local target rect for the highlighted area. SirkelEffect
    // resets this to a zero-size rect at the target screen's center (while
    // still hidden) at the start of every session, so the very first
    // direction picked grows out from the middle rather than gliding in
    // from wherever the previous session last left it.
    property real targetX: 0
    property real targetY: 0
    property real targetWidth: 0
    property real targetHeight: 0

    // easing.overshoot (only meaningful for the "overshoot" style) tones
    // the bounce down from OutBack's rather punchy default (~1.70158) —
    // full strength reads fine on the indicator's slim pointer, but felt
    // too aggressive on the outline's whole filled rect bulging past its
    // target size/position before settling.
    Behavior on targetX {
        enabled: root.animationDuration > 0
        NumberAnimation { duration: root.animationDuration; easing.type: root.easingType(); easing.overshoot: 0.6 }
    }
    Behavior on targetY {
        enabled: root.animationDuration > 0
        NumberAnimation { duration: root.animationDuration; easing.type: root.easingType(); easing.overshoot: 0.6 }
    }
    Behavior on targetWidth {
        enabled: root.animationDuration > 0
        NumberAnimation { duration: root.animationDuration; easing.type: root.easingType(); easing.overshoot: 0.6 }
    }
    Behavior on targetHeight {
        enabled: root.animationDuration > 0
        NumberAnimation { duration: root.animationDuration; easing.type: root.easingType(); easing.overshoot: 0.6 }
    }

    // Called once from SirkelEffect::finish() right as a snap commits
    // (before the overlay is hidden — see m_overlayHideTimer) — a brief
    // brightness pulse + scale pop on top of whatever's already drawn, to
    // mark the moment distinctly from the glide animation that got it
    // there.
    function playConfirmFlash() {
        if (!root.confirmFlashEnabled) {
            return;
        }
        confirmFlashAnim.restart();
        confirmPopAnim.restart();
    }

    // Called once from SirkelEffect::setDirection() the first time the
    // outline actually becomes visible each session — pops the fill in
    // (scale + fade from ~90%) instead of snapping straight to fully
    // drawn, as long as animations aren't fully off (OutlineAnimationDuration
    // 0 means instant everywhere, entrance included). Explicitly invoked
    // rather than keyed off `visible` changing: root.visible only ever
    // transitions false->true *once* in the plugin's entire lifetime
    // (SirkelEffect never actually calls setDirection(SnapDirection::None),
    // so the code path that would set it back to false is unreachable),
    // not once per session as an onVisibleChanged-driven version assumed —
    // that version only ever played on the very first snap after KWin
    // loaded the effect.
    function playEntrance() {
        if (root.animationDuration <= 0) {
            return;
        }
        fillRect.opacity = 0;
        fillRect.scale = 0.9;
        entranceAnim.restart();
    }

    ParallelAnimation {
        id: entranceAnim
        NumberAnimation { target: fillRect; property: "opacity"; to: 1; duration: 180; easing.type: Easing.OutCubic }
        NumberAnimation { target: fillRect; property: "scale"; to: 1; duration: 180; easing.type: Easing.OutBack }
    }

    SequentialAnimation {
        id: confirmPopAnim
        NumberAnimation { target: fillRect; property: "scale"; to: 1.06; duration: 80; easing.type: Easing.OutQuad }
        NumberAnimation { target: fillRect; property: "scale"; to: 1; duration: 180; easing.type: Easing.OutBack }
    }

    // Peak opacity and durations bumped up from the first version (0.55
    // peak / 70-260ms) — at the original values this was too easy to miss
    // (confirmed live), especially against a low Fill opacity.
    SequentialAnimation {
        id: confirmFlashAnim
        NumberAnimation { target: flashOverlay; property: "opacity"; to: 0.85; duration: 90; easing.type: Easing.OutQuad }
        NumberAnimation { target: flashOverlay; property: "opacity"; to: 0; duration: 320; easing.type: Easing.InQuad }
    }

    // A smooth-falloff alpha curve rather than a flat multiplier, sampled
    // at several stops below instead of just two — a 2-stop linear ramp
    // read as visibly banded/stepped at the low alpha values this feature
    // typically runs at (confirmed live). `pos` is 0..1 along the
    // gradient's own solid->faint axis (already flipped for
    // gradientSolidAtStart by the caller); eases out from a boosted peak
    // near 0 down to a dim tail near 1, same ~0.4-1.6x envelope as before.
    function gradientAlphaFactor(pos) {
        return 0.4 + (1 - pos * pos) * 1.2;
    }

    // A subtle sheen of the same hue, in place of the flat fill, when
    // gradientFillEnabled — same fillOpacity as its baseline, just not
    // uniform, and oriented per gradientHorizontal/gradientSolidAtStart
    // above. Pure paint change, no extra rendering cost either way.
    Gradient {
        id: fillGradient
        orientation: root.gradientHorizontal ? Gradient.Horizontal : Gradient.Vertical
        GradientStop { position: 0.00; color: Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, Math.min(1, root.fillOpacity / 100 * root.gradientAlphaFactor(root.gradientSolidAtStart ? 0.00 : 1.00))) }
        GradientStop { position: 0.25; color: Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, Math.min(1, root.fillOpacity / 100 * root.gradientAlphaFactor(root.gradientSolidAtStart ? 0.25 : 0.75))) }
        GradientStop { position: 0.50; color: Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, Math.min(1, root.fillOpacity / 100 * root.gradientAlphaFactor(0.50))) }
        GradientStop { position: 0.75; color: Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, Math.min(1, root.fillOpacity / 100 * root.gradientAlphaFactor(root.gradientSolidAtStart ? 0.75 : 0.25))) }
        GradientStop { position: 1.00; color: Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, Math.min(1, root.fillOpacity / 100 * root.gradientAlphaFactor(root.gradientSolidAtStart ? 1.00 : 0.00))) }
    }

    Rectangle {
        id: fillRect
        transformOrigin: Item.Center
        x: root.targetX + root.borderWidth / 2
        y: root.targetY + root.borderWidth / 2
        width: Math.max(0, root.targetWidth - root.borderWidth)
        height: Math.max(0, root.targetHeight - root.borderWidth)
        color: Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, root.fillOpacity / 100)
        gradient: root.gradientFillEnabled ? fillGradient : null
        border.color: root.borderColor
        border.width: root.borderWidth
        radius: root.cornerRadius

        // The confirm-flash's actual brightness pulse — a plain overlay on
        // top of the fill/border, opacity-animated by confirmFlashAnim.
        // anchors.fill + the same radius keeps it pixel-matched to fillRect
        // without duplicating its position math.
        Rectangle {
            id: flashOverlay
            anchors.fill: parent
            radius: parent.radius
            color: root.borderColor
            opacity: 0
        }
    }
}
