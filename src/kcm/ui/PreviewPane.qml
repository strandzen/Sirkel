import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.kirigami as Kirigami

// Live preview: the real Overlay (Outline + Indicator, the same files the
// effect uses) over the blurred wallpaper, cycling through a few picks so
// every visual setting can be seen without triggering Sirkel. The pane is
// the "screen": the overlay is laid out on a virtual screen and drawn at a
// fixed scale, so the indicator keeps the same size however wide the pane
// is; only the screen area (and with it the outline's placements) follows
// the pane.
Item {
    id: pane
    clip: true

    // Fixed drawing scale: 1 shows the indicator at exactly its real size.
    readonly property real scaleFactor: 1.0
    // Virtual screen: the pane's size at that scale.
    readonly property real screenW: Math.max(1, width / scaleFactor)
    readonly property real screenH: Math.max(1, height / scaleFactor)

    // The demo: picks in order, then a release (exit) and a restart.
    readonly property var demo: ["left", "topRight", "maximize", "bottom", "right"]
    property int step: -1

    function showStep(dir, first) {
        const ind = overlay.indicatorItem;
        const out = overlay.outlineItem;
        // Geometry, ring position and label come from the same C++ code the
        // effect uses.
        const p = kcm.previewPlacement(dir, screenW, screenH);
        ind.label = p.label;
        ind.ringIndex = p.ringIndex;
        ind.direction = dir;
        if (first) {
            // Grow out from the middle (there's no real window to grow from).
            out.jumpTo(screenW / 2, screenH / 2, 0, 0);
        }
        out.targetX = p.rect.x;
        out.targetY = p.rect.y;
        out.targetWidth = p.rect.width;
        out.targetHeight = p.rect.height;
        if (first) {
            if (kcm.settings.showIndicator) {
                ind.playEntrance();
            }
            if (kcm.settings.showOutline) {
                out.playEntrance();
            }
        }
    }

    function advance() {
        step += 1;
        if (step < demo.length) {
            showStep(demo[step], step === 0);
            ticker.restart();
        } else {
            overlay.playExit();
        }
    }

    function restart() {
        overlay.reset();
        step = -1;
        overlay.indicatorItem.intensity = 1;
        ticker.interval = 300;
        ticker.restart();
    }

    Timer {
        id: ticker
        interval: 1300
        onTriggered: {
            interval = 1300;
            pane.advance();
        }
    }

    Connections {
        target: overlay
        function onExitFinished() {
            pane.restart();
        }
    }

    // Visibility switches restart the demo so entrances replay correctly.
    Connections {
        target: kcm.settings
        function onShowIndicatorChanged() { pane.restart(); }
        function onShowOutlineChanged() { pane.restart(); }
    }

    Component.onCompleted: restart()

    // Background: the wallpaper, blurred and slightly darkened, so it reads
    // like frosted glass over the desktop. Falls back to the window color.
    Rectangle {
        anchors.fill: parent
        color: Kirigami.Theme.backgroundColor
    }

    Image {
        id: wallpaper
        anchors.fill: parent
        source: kcm.wallpaperUrl
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        // A small decode is plenty under a heavy blur.
        sourceSize.width: 960
        visible: status === Image.Ready
        // The blur needs a GPU renderer; on the software renderer the image
        // just shows unblurred.
        layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 1.0
            blurMax: 48
            brightness: -0.12
            saturation: 0.15
        }
    }

    Item {
        id: virtualScreen
        width: pane.screenW
        height: pane.screenH
        transform: Scale { xScale: pane.scaleFactor; yScale: pane.scaleFactor }

        Overlay {
            id: overlay
            motionScale: kcm.settings.outlineAnimationDuration / 75
            easing: kcm.settings.easing
            shadowEnabled: kcm.settings.shadow
            twoTone: kcm.settings.twoTone
        }
    }

    // Style, bound live to the settings.
    Binding { target: overlay.indicatorItem; property: "x"; value: (pane.screenW - kcm.settings.indicatorSize) / 2 }
    Binding { target: overlay.indicatorItem; property: "y"; value: (pane.screenH - kcm.settings.indicatorSize) / 2 }
    Binding { target: overlay.indicatorItem; property: "width"; value: kcm.settings.indicatorSize }
    Binding { target: overlay.indicatorItem; property: "height"; value: kcm.settings.indicatorSize }
    Binding { target: overlay.indicatorItem; property: "cornerRadius"; value: kcm.settings.indicatorCornerRadius }
    Binding { target: overlay.indicatorItem; property: "ringWidth"; value: kcm.settings.indicatorRingWidth }
    Binding { target: overlay.indicatorItem; property: "pointerLength"; value: kcm.settings.indicatorPointerLength }
    Binding { target: overlay.indicatorItem; property: "showText"; value: kcm.settings.showIndicatorText }
    Binding { target: overlay.indicatorItem; property: "highlightColor"; value: kcm.resolvedPointerColor }
    Binding { target: overlay.indicatorItem; property: "ringColor"; value: kcm.resolvedRingColor }
    Binding { target: overlay.indicatorItem; property: "baseOpacity"; value: kcm.settings.indicatorRingFillOpacity / 100 }
    // At least one real pixel once scaled down, or thin borders vanish (and
    // only some of their edges survive the rounding).
    Binding { target: overlay.outlineItem; property: "borderWidth"; value: kcm.settings.outlineBorderWidth > 0 ? Math.max(kcm.settings.outlineBorderWidth, 1 / Math.max(0.01, pane.scaleFactor)) : 0 }
    Binding { target: overlay.outlineItem; property: "borderColor"; value: kcm.resolvedOutlineColor }
    Binding { target: overlay.outlineItem; property: "cornerRadius"; value: kcm.settings.outlineCornerRadius }
    Binding { target: overlay.outlineItem; property: "fillOpacity"; value: kcm.settings.outlineFillOpacity }
}
