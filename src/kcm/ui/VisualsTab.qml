import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Visuals tab — merges the former IndicatorTab.qml and OutlineTab.qml into
// one tab with two sections, plus a shared "Motion" section at the end for
// the two settings (Animation duration, Easing) that were already shared
// backend values pushed to both parts — they used to be shown twice
// (once per old tab), which stops making sense once both live in the same
// tab. See InputTab.qml's header comment for the ScrollBar.AlwaysOn fix.
QQC2.ScrollView {
    id: root

    property real fieldWidth: Kirigami.Units.gridUnit * 14
    property real formWidth: Kirigami.Units.gridUnit * 30

    contentWidth: availableWidth
    QQC2.ScrollBar.vertical.policy: QQC2.ScrollBar.AlwaysOn

    Item {
        width: parent.width
        implicitHeight: visualsForm.implicitHeight + Kirigami.Units.largeSpacing * 2

        Kirigami.FormLayout {
            id: visualsForm
            width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, root.formWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Kirigami.Units.largeSpacing

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Indicator"
            }

            QQC2.Switch {
                Kirigami.FormData.label: "Visible:"
                text: "Show the indicator"
                checked: kcm.showIndicator
                onToggled: kcm.showIndicator = checked
            }

            QQC2.Switch {
                Kirigami.FormData.label: "Text:"
                text: "Show direction name"
                checked: kcm.showIndicatorText
                onToggled: kcm.showIndicatorText = checked
            }

            QQC2.Switch {
                Kirigami.FormData.label: "Position:"
                text: "Spawn at cursor"
                checked: kcm.indicatorFollowsMouse
                onToggled: kcm.indicatorFollowsMouse = checked
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Size (px):"
                Layout.preferredWidth: root.fieldWidth
                from: 60
                to: 400
                value: kcm.indicatorSize
                onValueModified: kcm.indicatorSize = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Corner radius (px):"
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: kcm.indicatorSize / 2
                value: kcm.indicatorCornerRadius
                onValueModified: kcm.indicatorCornerRadius = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Line thickness (px):"
                Layout.preferredWidth: root.fieldWidth
                from: 2
                to: 60
                value: kcm.indicatorRingWidth
                onValueModified: kcm.indicatorRingWidth = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Pointer length (px):"
                Layout.preferredWidth: root.fieldWidth
                from: 6
                to: 200
                value: kcm.indicatorPointerLength
                onValueModified: kcm.indicatorPointerLength = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Opacity (%):"
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 100
                value: kcm.indicatorRingFillOpacity
                onValueModified: kcm.indicatorRingFillOpacity = value

                QQC2.ToolTip.text: "Ring only — pointer and text always stay fully visible."
                QQC2.ToolTip.visible: hovered
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Outline"
            }

            QQC2.Switch {
                Kirigami.FormData.label: "Visible:"
                text: "Show the placement preview"
                checked: kcm.showOutline
                onToggled: kcm.showOutline = checked
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Border thickness (px):"
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 20
                value: kcm.outlineBorderWidth
                onValueModified: kcm.outlineBorderWidth = value

                QQC2.ToolTip.text: "0 hides the border, leaving just the fill."
                QQC2.ToolTip.visible: hovered
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Corner radius (px):"
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 60
                value: kcm.outlineCornerRadius
                onValueModified: kcm.outlineCornerRadius = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Opacity (%):"
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 100
                value: kcm.outlineFillOpacity
                onValueModified: kcm.outlineFillOpacity = value
            }

            QQC2.Switch {
                Kirigami.FormData.label: "Blur:"
                text: "Blur the desktop behind the fill"
                checked: kcm.outlineBlur
                onToggled: kcm.outlineBlur = checked

                QQC2.ToolTip.text: "Needs Desktop Effects' Blur enabled. Opacity above controls how much shows through."
                QQC2.ToolTip.visible: hovered
            }

            QQC2.Switch {
                Kirigami.FormData.label: "Confirm flash:"
                text: "Brief pulse on the fill right as it snaps"
                checked: kcm.outlineConfirmFlash
                onToggled: kcm.outlineConfirmFlash = checked

                QQC2.ToolTip.text: "Marks the moment it snaps, separate from the glide animation."
                QQC2.ToolTip.visible: hovered
            }

            QQC2.Switch {
                Kirigami.FormData.label: "Gradient fill:"
                text: "Subtle gradient instead of a flat fill"
                checked: kcm.outlineGradientFill
                onToggled: kcm.outlineGradientFill = checked

                QQC2.ToolTip.text: "Brighter top, dimmer bottom, same hue/opacity as flat."
                QQC2.ToolTip.visible: hovered
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Motion"
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Animation duration (ms):"
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 1000
                stepSize: 10
                value: kcm.outlineAnimationDuration
                onValueModified: kcm.outlineAnimationDuration = value

                QQC2.ToolTip.text: "0 = instant, no entrance pop-in. Higher = slower glide for both."
                QQC2.ToolTip.visible: hovered
            }

            QQC2.ComboBox {
                Kirigami.FormData.label: "Easing:"
                Layout.preferredWidth: root.fieldWidth
                Layout.minimumWidth: 0
                textRole: "text"
                model: [
                    { value: "linear", text: "Linear" },
                    { value: "smooth", text: "Smooth" },
                    { value: "overshoot", text: "Overshoot" },
                ]
                currentIndex: model.findIndex(m => m.value === kcm.easing)
                onActivated: kcm.easing = model[currentIndex].value

                QQC2.ToolTip.text: "Linear: constant speed. Smooth: eases out. Overshoot: eases past the target then settles back."
                QQC2.ToolTip.visible: hovered
            }
        }
    }
}
