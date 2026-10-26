// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// General: how Sirkel is triggered and how you pick a placement.
FormPage {
    id: root

    // Whether keyd is set up, as one status line instead of a warning
    // banner. The setup steps live on the About page.
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: true
        type: !kcm.triggerIsHyperkey ? Kirigami.MessageType.Information
            : kcm.keydOk ? Kirigami.MessageType.Positive : Kirigami.MessageType.Warning
        text: !kcm.triggerIsHyperkey
            ? i18n("Hold %1 to use Sirkel (set on the Shortcuts page). keyd isn't needed.", kcm.triggerText)
            : kcm.keydOk
                ? i18n("Hold Caps Lock to use Sirkel. keyd is running and maps Caps Lock to F24.")
                : i18n("keyd doesn't seem to be running with Caps Lock mapped to F24, so Caps Lock won't work yet. See About for the setup steps, or use a key combination instead (Shortcuts page).")
    }

    Kirigami.FormLayout {
        id: mainForm
        Layout.fillWidth: true

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Picking")
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("How you pick a direction:")
            Layout.preferredWidth: root.fieldWidth
            // A long option would otherwise widen the whole form.
            Layout.minimumWidth: 0
            textRole: "text"
            model: [
                { value: "box", text: i18n("Box (distance per axis)") },
                { value: "angle", text: i18n("Angle (eight slices)") },
            ]
            currentIndex: model.findIndex(m => m.value === kcm.settings.pickingMode)
            onActivated: kcm.settings.pickingMode = model[currentIndex].value

            QQC2.ToolTip.text: i18n("Box: move a set distance left/right or up/down; corners need both. Angle: move past one center radius, then the direction you moved picks one of eight slices, so corners are as easy as edges.")
            QQC2.ToolTip.visible: hovered
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Direction keys:")
            Layout.preferredWidth: root.fieldWidth
            Layout.minimumWidth: 0
            textRole: "text"
            // "none" = mouse only, no direction keys.
            model: kcm.keybindSchemes.map(scheme => ({
                value: scheme,
                text: ({ arrows: i18n("Arrows"), wasd: i18n("WASD"), hjkl: i18n("HJKL"), none: i18n("None (mouse only)") })[scheme] || scheme
            }))
            currentIndex: kcm.keybindSchemes.indexOf(kcm.settings.keybindScheme)
            onActivated: kcm.settings.keybindScheme = kcm.keybindSchemes[currentIndex]
        }

        QQC2.Switch {
            Kirigami.FormData.label: i18n("Scroll wheel:")
            text: i18n("Resize the target before releasing")
            checked: kcm.settings.scrollResizeEnabled
            onToggled: kcm.settings.scrollResizeEnabled = checked

            QQC2.ToolTip.text: i18n("Cycles the size each direction controls: 1/4, 1/3, 1/2, 2/3. Never carries over to the next pick.")
            QQC2.ToolTip.visible: hovered
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Target")
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Act on:")
            Layout.preferredWidth: root.fieldWidth
            Layout.minimumWidth: 0
            textRole: "text"
            model: [
                { value: false, text: i18n("The active window") },
                { value: true, text: i18n("The window under the cursor") },
            ]
            currentIndex: kcm.settings.targetWindowUnderCursor ? 1 : 0
            onActivated: kcm.settings.targetWindowUnderCursor = model[currentIndex].value

            QQC2.ToolTip.text: i18n("Under the cursor: point at a window and snap it without clicking it first. Falls back to the active window when the cursor isn't over one.")
            QQC2.ToolTip.visible: hovered
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Snap on:")
            Layout.preferredWidth: root.fieldWidth
            Layout.minimumWidth: 0
            textRole: "text"
            model: [
                { value: false, text: i18n("The window's own monitor") },
                { value: true, text: i18n("The monitor under the cursor") },
            ]
            currentIndex: kcm.settings.snapToCursorScreen ? 1 : 0
            onActivated: kcm.settings.snapToCursorScreen = model[currentIndex].value

            QQC2.ToolTip.text: i18n("Decided when you press the trigger; doesn't change if the cursor moves to another monitor mid-pick.")
            QQC2.ToolTip.visible: hovered
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Gaps")
        }

        SliderRow {
            Kirigami.FormData.label: i18n("Horizontal:")
            fieldWidth: root.fieldWidth
            from: 0
            to: 120
            unit: " px"
            value: kcm.settings.paddingHorizontal
            onMoved: (v) => kcm.settings.paddingHorizontal = v
        }

        SliderRow {
            Kirigami.FormData.label: i18n("Vertical:")
            fieldWidth: root.fieldWidth
            from: 0
            to: 120
            unit: " px"
            value: kcm.settings.paddingVertical
            onMoved: (v) => kcm.settings.paddingVertical = v
        }
    }

    AdvancedFold {
        twin: mainForm

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Picking distances")
        }

        QQC2.SpinBox {
            Kirigami.FormData.label: i18n("Center radius (px):")
            Layout.preferredWidth: root.fieldWidth
            visible: kcm.settings.pickingMode === "angle"
            from: 5
            to: 300
            value: kcm.settings.centerRadius
            onValueModified: kcm.settings.centerRadius = value

            QQC2.ToolTip.text: i18n("How far the mouse has to move before a direction is picked. Inside this circle is Maximize.")
            QQC2.ToolTip.visible: hovered
        }

        QQC2.SpinBox {
            Kirigami.FormData.label: i18n("Deadzone (px):")
            Layout.preferredWidth: root.fieldWidth
            visible: kcm.settings.pickingMode !== "angle"
            from: 1
            to: 500
            value: kcm.settings.deadzone
            onValueModified: kcm.settings.deadzone = value

            QQC2.ToolTip.text: i18n("How far the mouse has to move from the center, left/right or up/down, before that direction counts. Both directions for a corner.")
            QQC2.ToolTip.visible: hovered
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Trigger")
        }

        QQC2.SpinBox {
            Kirigami.FormData.label: i18n("Auto-cancel after (s):")
            Layout.preferredWidth: root.fieldWidth
            from: 0
            to: 60
            value: kcm.settings.sessionTimeout
            onValueModified: kcm.settings.sessionTimeout = value

            QQC2.ToolTip.text: i18n("Cancels if nothing happens for this long while the trigger is held, in case its release was missed. 0 = never.")
            QQC2.ToolTip.visible: hovered
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Excluded apps")
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Add:")

            QQC2.TextField {
                id: newExcludedApp
                Layout.preferredWidth: root.fieldWidth - addButton.implicitWidth - Kirigami.Units.smallSpacing
                placeholderText: i18n("e.g. steam or org.kde.konsole")
                onAccepted: addButton.clicked()
            }
            QQC2.ToolButton {
                id: addButton
                icon.name: "list-add"
                enabled: newExcludedApp.text.trim().length > 0
                onClicked: {
                    const entry = newExcludedApp.text.trim();
                    // Matching ignores case, so "Steam" and "steam" are one app.
                    const known = kcm.settings.excludedApps.some(app => app.toLowerCase() === entry.toLowerCase());
                    if (entry.length > 0 && !known) {
                        kcm.settings.excludedApps = kcm.settings.excludedApps.concat([entry]);
                    }
                    newExcludedApp.clear();
                }
                QQC2.ToolTip.text: i18n("Ignore this app. Matched against the app's desktop file name or window class (Window Rules in System Settings can show both).")
                QQC2.ToolTip.visible: hovered
            }
        }

        // One form row holding the whole list: Kirigami.FormLayout places
        // items a Repeater creates directly inside it after all its own rows.
        ColumnLayout {
            Kirigami.FormData.label: i18n("Ignored:")
            visible: kcm.settings.excludedApps.length > 0
            spacing: 0

            Repeater {
                model: kcm.settings.excludedApps
                delegate: RowLayout {
                    required property string modelData
                    required property int index
                    QQC2.Label {
                        Layout.preferredWidth: root.fieldWidth - removeButton.implicitWidth - Kirigami.Units.smallSpacing
                        text: modelData
                        elide: Text.ElideRight
                    }
                    QQC2.ToolButton {
                        id: removeButton
                        icon.name: "list-remove"
                        onClicked: {
                            const list = kcm.settings.excludedApps.slice();
                            list.splice(index, 1);
                            kcm.settings.excludedApps = list;
                        }
                        QQC2.ToolTip.text: i18n("Remove")
                        QQC2.ToolTip.visible: hovered
                    }
                }
            }
        }
    }
}
