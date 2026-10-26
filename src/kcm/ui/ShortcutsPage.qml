// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kquickcontrols as KQuickControls

// Shortcuts: what starts Sirkel (Caps Lock via keyd, or a key combination
// for setups without keyd), direct shortcuts for each action, and the keys
// that work while holding the trigger. Shortcut changes apply immediately;
// they're stored by KGlobalAccel, also editable in System Settings ->
// Shortcuts -> Sirkel.
FormPage {
    id: root

    // Whether the combination option is selected. Kept separately from the
    // stored shortcut: choosing it doesn't change anything until a
    // combination is recorded (there's no default).
    property bool comboMode: !kcm.triggerIsHyperkey

    Kirigami.FormLayout {
        id: mainForm
        Layout.fillWidth: true

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Trigger")
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Start Sirkel with:")
            Layout.preferredWidth: root.fieldWidth
            Layout.minimumWidth: 0
            model: [i18n("Caps Lock (via keyd)"), i18n("A key combination")]
            currentIndex: root.comboMode ? 1 : 0
            onActivated: (index) => {
                root.comboMode = index === 1;
                if (!root.comboMode) {
                    kcm.useHyperkeyTrigger();
                }
            }
        }

        KQuickControls.KeySequenceItem {
            Kirigami.FormData.label: i18n("Combination:")
            visible: root.comboMode
            modifierlessAllowed: false
            checkForConflictsAgainst: KQuickControls.ShortcutType.GlobalShortcuts | KQuickControls.ShortcutType.StandardShortcuts
            keySequence: kcm.shortcutsRevision, kcm.triggerKind === "combination" ? kcm.shortcut(kcm.triggerId) : ""
            onKeySequenceModified: {
                if (keySequence.toString().length > 0) {
                    kcm.setShortcut(kcm.triggerId, keySequence);
                } else {
                    // Cleared: back to Caps Lock rather than no trigger at all.
                    kcm.useHyperkeyTrigger();
                    root.comboMode = false;
                }
            }
        }

        QQC2.Label {
            Kirigami.FormData.label: " "
            Layout.preferredWidth: root.fieldWidth
            wrapMode: Text.WordWrap
            color: Kirigami.Theme.disabledTextColor
            text: {
                if (!root.comboMode) {
                    return i18n("Needs keyd to send Caps Lock as F24; see About.");
                }
                if (kcm.triggerKind === "combination") {
                    return i18n("Hold the combination, pick a direction, release either key to snap. keyd isn't needed.");
                }
                return kcm.triggerKind === "capslock"
                    ? i18n("Record a combination, for example Meta+Z. Until then Caps Lock stays the trigger.")
                    : i18n("No trigger is set yet. Record a combination, for example Meta+Z.");
            }
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("While holding the trigger")
        }

        QQC2.Label {
            Kirigami.FormData.label: i18n("Move the mouse:")
            text: i18n("Pick a direction")
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Direction keys:")
            text: i18n("Pick a direction; two at once for a corner")
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Same key again:")
            text: i18n("Change the size (1/2, 1/3, 1/4, 2/3)")
        }
        // Rebindable, also under Sirkel in System Settings -> Shortcuts. Only
        // active while the trigger is held, so a plain key like Enter is
        // fine. Two explicit rows rather than a Repeater, which
        // Kirigami.FormLayout would move to the end of the form.
        KQuickControls.KeySequenceItem {
            Kirigami.FormData.label: kcm.holdKeyActions[0].text + ":"
            modifierlessAllowed: true
            checkForConflictsAgainst: KQuickControls.ShortcutType.GlobalShortcuts
            keySequence: kcm.shortcutsRevision, kcm.shortcut(kcm.holdKeyActions[0].id)
            onKeySequenceModified: kcm.setShortcut(kcm.holdKeyActions[0].id, keySequence)
        }
        KQuickControls.KeySequenceItem {
            Kirigami.FormData.label: kcm.holdKeyActions[1].text + ":"
            modifierlessAllowed: true
            checkForConflictsAgainst: KQuickControls.ShortcutType.GlobalShortcuts
            keySequence: kcm.shortcutsRevision, kcm.shortcut(kcm.holdKeyActions[1].id)
            onKeySequenceModified: kcm.setShortcut(kcm.holdKeyActions[1].id, keySequence)
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Cancel:")
            text: i18n("Esc")
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Scroll wheel:")
            text: kcm.settings.scrollResizeEnabled ? i18n("Resize the target") : i18n("Resize the target (turned off on General)")
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Release:")
            text: i18n("Snap the window")
        }
    }

    // Its own form: Kirigami.FormLayout puts Repeater-created rows after all
    // its other rows, so the repeated rows need a form where they come last.
    Kirigami.FormLayout {
        Layout.fillWidth: true
        twinFormLayouts: [mainForm]

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Direct shortcuts")
        }

        QQC2.Label {
            Kirigami.FormData.label: " "
            Layout.preferredWidth: root.fieldWidth
            wrapMode: Text.WordWrap
            color: Kirigami.Theme.disabledTextColor
            text: i18n("Act on the active window at once, without the indicator. None are set by default.")
        }

        Repeater {
            model: kcm.shortcutActions
            delegate: KQuickControls.KeySequenceItem {
                required property var modelData
                Kirigami.FormData.label: modelData.text + ":"
                checkForConflictsAgainst: KQuickControls.ShortcutType.GlobalShortcuts | KQuickControls.ShortcutType.StandardShortcuts
                // shortcutsRevision makes the row re-read after any change,
                // e.g. a conflict reassignment that took this row's keys.
                keySequence: kcm.shortcutsRevision, kcm.shortcut(modelData.id)
                onKeySequenceModified: kcm.setShortcut(modelData.id, keySequence)
            }
        }
    }

    QQC2.Button {
        Layout.alignment: Qt.AlignHCenter
        text: i18n("Open Shortcuts settings…")
        icon.name: "preferences-desktop-keyboard-shortcut"
        onClicked: kcm.openShortcutsSettings()

        QQC2.ToolTip.text: i18n("All of these are also under Sirkel in System Settings → Shortcuts.")
        QQC2.ToolTip.visible: hovered
    }

    // Re-read after any change (including ones made elsewhere this session).
    Connections {
        target: kcm
        function onShortcutsChanged() {
            root.comboMode = root.comboMode || !kcm.triggerIsHyperkey;
        }
    }
}
