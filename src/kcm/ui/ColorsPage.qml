import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Colors: the outline, the indicator's pointer and its ring, each with its
// own source.
FormPage {
    id: root

    Kirigami.FormLayout {
        id: mainForm
        Layout.fillWidth: true

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Outline")
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

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Pointer")
        }

        ColorSourceCombo {
            Kirigami.FormData.label: i18n("Color from:")
            Layout.preferredWidth: root.fieldWidth
            sourceValue: kcm.settings.indicatorPointerColorSource
            onSourcePicked: (value) => kcm.settings.indicatorPointerColorSource = value
        }

        ColorPickerRow {
            Kirigami.FormData.label: i18n("Color:")
            colorValue: kcm.resolvedPointerColor
            target: "pointer"
            popup: colorPopup
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Ring")
        }

        // No wallpaper option: the ring is meant to keep its own neutral
        // identity. The theme option is a dark, muted accent.
        ColorSourceCombo {
            Kirigami.FormData.label: i18n("Color from:")
            Layout.preferredWidth: root.fieldWidth
            sourceOptions: [
                { value: "manual", text: i18n("Custom") },
                { value: "theme", text: i18n("Theme accent, muted (live)") },
            ]
            sourceValue: kcm.settings.indicatorRingColorSource
            onSourcePicked: (value) => kcm.settings.indicatorRingColorSource = value
        }

        ColorPickerRow {
            Kirigami.FormData.label: i18n("Color:")
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
