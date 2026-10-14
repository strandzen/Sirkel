import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Input tab content (renamed from BehaviorTab.qml as part of the Input /
// Visuals / Colors tab restructure) — see BehaviorTab.qml's original
// header comment history in timeline.md for why tabs are split files.
// ScrollBar.vertical.policy: AlwaysOn reserves the scrollbar's own width
// permanently, whether or not this tab's content is currently tall enough
// to need scrolling — without it, a tab that happens to overflow loses a
// few pixels of usable width right when its fields are closest to
// wrapping to label-above-field layout, while a shorter tab that never
// scrolls doesn't lose that width and stays inline (confirmed live: this
// was exactly why this tab wrapped and others didn't).
QQC2.ScrollView {
    id: root

    property real fieldWidth: Kirigami.Units.gridUnit * 14
    property real formWidth: Kirigami.Units.gridUnit * 30

    contentWidth: availableWidth
    QQC2.ScrollBar.vertical.policy: QQC2.ScrollBar.AlwaysOn

    Item {
        width: parent.width
        implicitHeight: keydWarning.visible
            ? keydWarning.implicitHeight + Kirigami.Units.largeSpacing * 3 + inputForm.implicitHeight
            : inputForm.implicitHeight + Kirigami.Units.largeSpacing * 2

        Kirigami.InlineMessage {
            id: keydWarning
            width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, root.formWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Kirigami.Units.largeSpacing
            visible: !kcm.keydOk
            type: Kirigami.MessageType.Warning
            text: "keyd doesn't seem to be running with Caps Lock mapped to F24 - the hyperkey won't work until that's fixed. See the README's \"Configure keyd\" step. (Checked once per System Settings launch - reopen it after fixing to clear this.)"
        }

        Kirigami.FormLayout {
            id: inputForm
            width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, root.formWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: keydWarning.visible ? keydWarning.bottom : parent.top
            anchors.topMargin: Kirigami.Units.largeSpacing

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Mouse movement deadzone"
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Horizontal (px):"
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 500
                value: kcm.horizontalDeadzone
                onValueModified: kcm.horizontalDeadzone = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Vertical (px):"
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 500
                value: kcm.verticalDeadzone
                onValueModified: kcm.verticalDeadzone = value
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Padding"
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Horizontal (px):"
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 200
                value: kcm.paddingHorizontal
                onValueModified: kcm.paddingHorizontal = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Vertical (px):"
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 200
                value: kcm.paddingVertical
                onValueModified: kcm.paddingVertical = value
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Monitors"
            }

            QQC2.Switch {
                Kirigami.FormData.label: "Targeting:"
                text: "Snap to cursor's monitor"
                checked: kcm.snapToCursorScreen
                onToggled: kcm.snapToCursorScreen = checked

                QQC2.ToolTip.text: "Set when you press the hyperkey — doesn't update if you change monitors mid-drag."
                QQC2.ToolTip.visible: hovered
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Keybinds"
            }

            QQC2.ComboBox {
                Kirigami.FormData.label: "Movement keys:"
                Layout.preferredWidth: root.fieldWidth
                // See ColorSourceCombo.qml's comment on the same property -
                // "None (mouse only)" is long enough to otherwise push this
                // row (and with it, per Kirigami.FormLayout's shared
                // column-width budget, every row in the form) wider than
                // intended, forcing the whole tab to wrap to label-above-
                // field layout (confirmed live).
                Layout.minimumWidth: 0
                textRole: "text"
                // "none" disables direction-key bindings entirely
                // for users who only want to drag the mouse — see
                // axisForArmedKey() in sirkeleffect.cpp.
                model: kcm.keybindSchemes.map(scheme => ({
                    value: scheme,
                    text: ({ arrows: "Arrows", wasd: "WASD", hjkl: "HJKL", none: "None (mouse only)" })[scheme] || scheme
                }))
                currentIndex: kcm.keybindSchemes.indexOf(kcm.keybindScheme)
                onActivated: kcm.keybindScheme = kcm.keybindSchemes[currentIndex]
            }

            QQC2.ComboBox {
                Kirigami.FormData.label: "Hyperkey trigger:"
                Layout.preferredWidth: root.fieldWidth
                Layout.minimumWidth: 0
                textRole: "text"
                // "delay" needs to watch for the key's release
                // before committing to a session — see TriggerPhase
                // in sirkeleffect.h. (A third mode, "doubleclick",
                // existed briefly but was cut as the most complex
                // trigger path for the least-used of the three.)
                model: [
                    { value: "instant", text: "Instant" },
                    { value: "delay", text: "Delay" },
                ]
                currentIndex: model.findIndex(m => m.value === kcm.triggerMode)
                onActivated: kcm.triggerMode = model[currentIndex].value

                QQC2.ToolTip.text: "Instant: arms immediately. Delay: must hold before it arms."
                QQC2.ToolTip.visible: hovered
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Delay (ms):"
                Layout.preferredWidth: root.fieldWidth
                visible: kcm.triggerMode === "delay"
                from: 0
                to: 1000
                stepSize: 50
                value: kcm.triggerDelay
                onValueModified: kcm.triggerDelay = value
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Scroll-sizing"
            }

            QQC2.Switch {
                Kirigami.FormData.label: "Scroll-sizing:"
                text: "Use scrollwheel to resize"
                checked: kcm.scrollResizeEnabled
                onToggled: kcm.scrollResizeEnabled = checked

                QQC2.ToolTip.text: "Resizes the one dimension each direction already controls, cycling 1/4 → 1/3 → 1/2 → 2/3. Never carries over to the next pick."
                QQC2.ToolTip.visible: hovered
            }
        }
    }
}
