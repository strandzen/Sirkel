import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Visuals tab: Indicator and Outline sections, plus a shared Motion section
// for settings that apply to both.
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
                Kirigami.FormData.label: i18n("Indicator")
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Visible:")
                text: i18n("Show the indicator")
                checked: kcm.settings.showIndicator
                onToggled: kcm.settings.showIndicator = checked
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Text:")
                text: i18n("Show direction name")
                checked: kcm.settings.showIndicatorText
                onToggled: kcm.settings.showIndicatorText = checked
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Position:")
                text: i18n("Spawn at cursor")
                checked: kcm.settings.indicatorFollowsMouse
                onToggled: kcm.settings.indicatorFollowsMouse = checked
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Size (px):")
                Layout.preferredWidth: root.fieldWidth
                from: 60
                to: 400
                value: kcm.settings.indicatorSize
                onValueModified: kcm.settings.indicatorSize = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Corner radius (px):")
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: kcm.settings.indicatorSize / 2
                value: kcm.settings.indicatorCornerRadius
                onValueModified: kcm.settings.indicatorCornerRadius = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Line thickness (px):")
                Layout.preferredWidth: root.fieldWidth
                from: 2
                to: 60
                value: kcm.settings.indicatorRingWidth
                onValueModified: kcm.settings.indicatorRingWidth = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Pointer length (px):")
                Layout.preferredWidth: root.fieldWidth
                from: 6
                to: 200
                value: kcm.settings.indicatorPointerLength
                onValueModified: kcm.settings.indicatorPointerLength = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Opacity (%):")
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 100
                value: kcm.settings.indicatorRingFillOpacity
                onValueModified: kcm.settings.indicatorRingFillOpacity = value

                QQC2.ToolTip.text: i18n("Ring only — pointer and text always stay fully visible.")
                QQC2.ToolTip.visible: hovered
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Outline")
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Visible:")
                text: i18n("Show the placement preview")
                checked: kcm.settings.showOutline
                onToggled: kcm.settings.showOutline = checked
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Border thickness (px):")
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 20
                value: kcm.settings.outlineBorderWidth
                onValueModified: kcm.settings.outlineBorderWidth = value

                QQC2.ToolTip.text: i18n("0 hides the border, leaving just the fill.")
                QQC2.ToolTip.visible: hovered
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Corner radius (px):")
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 60
                value: kcm.settings.outlineCornerRadius
                onValueModified: kcm.settings.outlineCornerRadius = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Opacity (%):")
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 100
                value: kcm.settings.outlineFillOpacity
                onValueModified: kcm.settings.outlineFillOpacity = value
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Blur:")
                text: i18n("Blur the desktop behind the fill")
                checked: kcm.settings.outlineBlur
                onToggled: kcm.settings.outlineBlur = checked

                QQC2.ToolTip.text: i18n("Needs Desktop Effects' Blur enabled. Opacity above controls how much shows through.")
                QQC2.ToolTip.visible: hovered
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Confirm flash:")
                text: i18n("Brief pulse on the fill right as it snaps")
                checked: kcm.settings.outlineConfirmFlash
                onToggled: kcm.settings.outlineConfirmFlash = checked

                QQC2.ToolTip.text: i18n("Marks the moment it snaps, separate from the glide animation.")
                QQC2.ToolTip.visible: hovered
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Gradient fill:")
                text: i18n("Subtle gradient instead of a flat fill")
                checked: kcm.settings.outlineGradientFill
                onToggled: kcm.settings.outlineGradientFill = checked

                QQC2.ToolTip.text: i18n("Solid toward the edge the window snaps against, fading away from it. Same hue and opacity as flat.")
                QQC2.ToolTip.visible: hovered
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Style")
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Shadow:")
                text: i18n("Soft shadow under indicator and outline")
                checked: kcm.settings.shadow
                onToggled: kcm.settings.shadow = checked
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Two-tone:")
                text: i18n("Shade pointer and fill into a darker color")
                checked: kcm.settings.twoTone
                onToggled: kcm.settings.twoTone = checked

                QQC2.ToolTip.text: i18n("Works together with Gradient fill: the fade sets the transparency, two-tone the color.")
                QQC2.ToolTip.visible: hovered
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Motion")
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Animation duration (ms):")
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 1000
                stepSize: 10
                value: kcm.settings.outlineAnimationDuration
                onValueModified: kcm.settings.outlineAnimationDuration = value

                QQC2.ToolTip.text: i18n("Sets the pace for every animation (glide, pointer slide, entrance, exit). Also follows the global animation speed. 0 = instant.")
                QQC2.ToolTip.visible: hovered
            }

            QQC2.ComboBox {
                Kirigami.FormData.label: i18n("Easing:")
                Layout.preferredWidth: root.fieldWidth
                Layout.minimumWidth: 0
                textRole: "text"
                model: [
                    { value: "fluid", text: i18n("Fluid") },
                    { value: "linear", text: i18n("Linear") },
                    { value: "smooth", text: i18n("Smooth") },
                    { value: "overshoot", text: i18n("Overshoot") },
                ]
                currentIndex: model.findIndex(m => m.value === kcm.settings.easing)
                onActivated: kcm.settings.easing = model[currentIndex].value

                QQC2.ToolTip.text: i18n("Fluid: fast start, long gentle settle, no bounce. Linear: constant speed. Smooth: plain ease out. Overshoot: eases past the target then settles back.")
                QQC2.ToolTip.visible: hovered
            }
        }
    }
}
