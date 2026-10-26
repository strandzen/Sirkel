// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Ring")
        }

        // The ring's theme option is a dark, muted version of the accent,
        // so the pointer stands out against it.
        ColorSourceCombo {
            Kirigami.FormData.label: i18n("Color from:")
            Layout.preferredWidth: root.fieldWidth
            sourceOptions: [
                { value: "manual", text: i18n("Custom") },
                { value: "theme", text: i18n("Theme accent, muted") },
            ]
            sourceValue: kcm.settings.indicatorRingColorSource
            onSourcePicked: (value) => kcm.settings.indicatorRingColorSource = value
        }

        ColorPickerRow {
            Kirigami.FormData.label: i18n("Color:")
            colorValue: kcm.resolvedRingColor
            target: "ring"
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
}
