import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Appearance: what the indicator and the outline preview look like, and how
// they move.
FormPage {
    id: root

    Kirigami.FormLayout {
        id: mainForm
        Layout.fillWidth: true

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Indicator")
        }

        QQC2.Switch {
            Kirigami.FormData.label: i18n("Show:")
            text: i18n("Show the direction indicator")
            checked: kcm.settings.showIndicator
            onToggled: kcm.settings.showIndicator = checked
        }

        QQC2.Switch {
            Kirigami.FormData.label: i18n("Position:")
            text: i18n("Appear at the cursor (otherwise screen center)")
            checked: kcm.settings.indicatorFollowsMouse
            onToggled: kcm.settings.indicatorFollowsMouse = checked
        }

        SliderRow {
            Kirigami.FormData.label: i18n("Size:")
            fieldWidth: root.fieldWidth
            from: 60
            to: 400
            unit: " px"
            value: kcm.settings.indicatorSize
            onMoved: (v) => kcm.settings.indicatorSize = v
        }

        SliderRow {
            Kirigami.FormData.label: i18n("Thickness:")
            fieldWidth: root.fieldWidth
            from: 2
            // A ring thicker than half the indicator would fill it.
            to: Math.max(2, Math.min(60, kcm.settings.indicatorSize / 2))
            unit: " px"
            value: kcm.settings.indicatorRingWidth
            onMoved: (v) => kcm.settings.indicatorRingWidth = v
        }

        SliderRow {
            Kirigami.FormData.label: i18n("Roundness:")
            fieldWidth: root.fieldWidth
            from: 0
            to: Math.max(1, kcm.settings.indicatorSize / 2)
            unit: " px"
            value: kcm.settings.indicatorCornerRadius
            onMoved: (v) => kcm.settings.indicatorCornerRadius = v
        }

        SliderRow {
            Kirigami.FormData.label: i18n("Ring opacity:")
            fieldWidth: root.fieldWidth
            from: 0
            to: 100
            unit: " %"
            value: kcm.settings.indicatorRingFillOpacity
            onMoved: (v) => kcm.settings.indicatorRingFillOpacity = v
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Outline preview")
        }

        QQC2.Switch {
            Kirigami.FormData.label: i18n("Show:")
            text: i18n("Show where the window will go")
            checked: kcm.settings.showOutline
            onToggled: kcm.settings.showOutline = checked
        }

        SliderRow {
            Kirigami.FormData.label: i18n("Border:")
            fieldWidth: root.fieldWidth
            from: 0
            to: 20
            unit: " px"
            value: kcm.settings.outlineBorderWidth
            onMoved: (v) => kcm.settings.outlineBorderWidth = v
        }

        SliderRow {
            Kirigami.FormData.label: i18n("Roundness:")
            fieldWidth: root.fieldWidth
            from: 0
            to: 60
            unit: " px"
            value: kcm.settings.outlineCornerRadius
            onMoved: (v) => kcm.settings.outlineCornerRadius = v
        }

        SliderRow {
            Kirigami.FormData.label: i18n("Fill opacity:")
            fieldWidth: root.fieldWidth
            from: 0
            to: 100
            unit: " %"
            value: kcm.settings.outlineFillOpacity
            onMoved: (v) => kcm.settings.outlineFillOpacity = v
        }

        QQC2.Switch {
            Kirigami.FormData.label: i18n("Blur:")
            text: i18n("Blur the desktop behind the fill")
            checked: kcm.settings.outlineBlur
            onToggled: kcm.settings.outlineBlur = checked

            QQC2.ToolTip.text: i18n("Needs Desktop Effects' Blur enabled. Fill opacity controls how much shows through.")
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

    AdvancedFold {
        twin: mainForm

        SliderRow {
            Kirigami.FormData.label: i18n("Pointer length:")
            fieldWidth: root.fieldWidth
            from: 6
            to: 200
            unit: " px"
            value: kcm.settings.indicatorPointerLength
            onMoved: (v) => kcm.settings.indicatorPointerLength = v
        }

        QQC2.Switch {
            Kirigami.FormData.label: i18n("Label:")
            text: i18n("Show the direction name inside the indicator")
            checked: kcm.settings.showIndicatorText
            onToggled: kcm.settings.showIndicatorText = checked
        }
    }
}
