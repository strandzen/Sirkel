import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Colors: one accent color for everything by default; separate colors for
// the indicator's pointer and ring are opt-in. "Use separate colors" is
// LinkIndicatorColors inverted.
FormPage {
    id: root

    Kirigami.FormLayout {
        id: mainForm
        Layout.fillWidth: true

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Accent")
        }

        ColorSourceCombo {
            Kirigami.FormData.label: i18n("Color from:")
            Layout.preferredWidth: root.fieldWidth
            sourceValue: kcm.settings.outlineColorSource
            onSourcePicked: (value) => kcm.settings.outlineColorSource = value
        }

        ColorPickerRow {
            Kirigami.FormData.label: i18n("Color:")
            colorValue: kcm.resolvedOutlineColor
            target: "outline"
            popup: colorPopup
        }

        QQC2.Switch {
            Kirigami.FormData.label: i18n("Indicator:")
            text: i18n("Use separate colors for pointer and ring")
            checked: !kcm.settings.linkIndicatorColors
            onToggled: kcm.settings.linkIndicatorColors = !checked
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Pointer")
            visible: !kcm.settings.linkIndicatorColors
        }

        ColorSourceCombo {
            Kirigami.FormData.label: i18n("Color from:")
            Layout.preferredWidth: root.fieldWidth
            visible: !kcm.settings.linkIndicatorColors
            sourceValue: kcm.settings.indicatorPointerColorSource
            onSourcePicked: (value) => kcm.settings.indicatorPointerColorSource = value
        }

        ColorPickerRow {
            Kirigami.FormData.label: i18n("Color:")
            visible: !kcm.settings.linkIndicatorColors
            colorValue: kcm.resolvedPointerColor
            target: "pointer"
            popup: colorPopup
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Ring")
            visible: !kcm.settings.linkIndicatorColors
        }

        // No wallpaper option: the ring is meant to keep its own neutral
        // identity.
        ColorSourceCombo {
            Kirigami.FormData.label: i18n("Color from:")
            Layout.preferredWidth: root.fieldWidth
            visible: !kcm.settings.linkIndicatorColors
            sourceOptions: [
                { value: "manual", text: i18n("Custom") },
                { value: "theme", text: i18n("Theme accent (live)") },
            ]
            sourceValue: kcm.settings.indicatorRingColorSource
            onSourcePicked: (value) => kcm.settings.indicatorRingColorSource = value
        }

        ColorPickerRow {
            Kirigami.FormData.label: i18n("Color:")
            visible: !kcm.settings.linkIndicatorColors
            colorValue: kcm.resolvedRingColor
            target: "ring"
            popup: colorPopup
        }

        QQC2.Button {
            Kirigami.FormData.label: " "
            text: i18n("Reset colors to defaults")
            icon.name: "edit-reset"
            onClicked: kcm.resetColors()

            QQC2.ToolTip.text: i18n("Resets just this page. The Defaults button resets everything.")
            QQC2.ToolTip.visible: hovered
        }
    }

    AdvancedFold {
        twin: mainForm

        QQC2.TextField {
            Kirigami.FormData.label: i18n("Wallpaper color file:")
            Layout.preferredWidth: root.fieldWidth
            placeholderText: i18n("Empty: pywal's default output")
            text: kcm.settings.wallpaperColorPath
            onEditingFinished: kcm.settings.wallpaperColorPath = text

            QQC2.ToolTip.text: i18n("Used when a color comes from the wallpaper. Empty = pywal's default output. A path = a file with one hex color (matugen, etc.).")
            QQC2.ToolTip.visible: hovered
        }
    }

    ColorPopup {
        id: colorPopup
        parent: root
    }
}
