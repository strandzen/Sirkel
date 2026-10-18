import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Input tab: trigger, deadzones, padding, keys, scroll-sizing.
// ScrollBar AlwaysOn reserves the scrollbar's width permanently, so a tab
// that overflows doesn't lose width and wrap to label-above-field layout.
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
            text: i18n("keyd doesn't seem to be running with Caps Lock mapped to F24 - the hyperkey won't work until that's fixed. See the README's \"Configure keyd\" step. (Checked once per System Settings launch - reopen it after fixing to clear this.)")
        }

        Kirigami.FormLayout {
            id: inputForm
            width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, root.formWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: keydWarning.visible ? keydWarning.bottom : parent.top
            anchors.topMargin: Kirigami.Units.largeSpacing

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Mouse movement deadzone")
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Horizontal (px):")
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 500
                value: kcm.settings.horizontalDeadzone
                onValueModified: kcm.settings.horizontalDeadzone = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Vertical (px):")
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 500
                value: kcm.settings.verticalDeadzone
                onValueModified: kcm.settings.verticalDeadzone = value
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Padding")
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Horizontal (px):")
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 200
                value: kcm.settings.paddingHorizontal
                onValueModified: kcm.settings.paddingHorizontal = value
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Vertical (px):")
                Layout.preferredWidth: root.fieldWidth
                from: 0
                to: 200
                value: kcm.settings.paddingVertical
                onValueModified: kcm.settings.paddingVertical = value
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Monitors")
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Targeting:")
                text: i18n("Snap to cursor's monitor")
                checked: kcm.settings.snapToCursorScreen
                onToggled: kcm.settings.snapToCursorScreen = checked

                QQC2.ToolTip.text: i18n("Set when you press the hyperkey — doesn't update if you change monitors mid-drag.")
                QQC2.ToolTip.visible: hovered
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Keybinds")
            }

            QQC2.ComboBox {
                Kirigami.FormData.label: i18n("Movement keys:")
                Layout.preferredWidth: root.fieldWidth
                // See ColorSourceCombo.qml: keeps a long option from widening the form.
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

            QQC2.ComboBox {
                Kirigami.FormData.label: i18n("Hyperkey trigger:")
                Layout.preferredWidth: root.fieldWidth
                Layout.minimumWidth: 0
                textRole: "text"
                model: [
                    { value: "instant", text: i18n("Instant") },
                    { value: "delay", text: i18n("Delay") },
                ]
                currentIndex: model.findIndex(m => m.value === kcm.settings.triggerMode)
                onActivated: kcm.settings.triggerMode = model[currentIndex].value

                QQC2.ToolTip.text: i18n("Instant: arms immediately. Delay: must hold before it arms.")
                QQC2.ToolTip.visible: hovered
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Delay (ms):")
                Layout.preferredWidth: root.fieldWidth
                visible: kcm.settings.triggerMode === "delay"
                from: 0
                to: 1000
                stepSize: 50
                value: kcm.settings.triggerDelay
                onValueModified: kcm.settings.triggerDelay = value
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Scroll-sizing")
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Scroll-sizing:")
                text: i18n("Use scrollwheel to resize")
                checked: kcm.settings.scrollResizeEnabled
                onToggled: kcm.settings.scrollResizeEnabled = checked

                QQC2.ToolTip.text: i18n("Resizes the one dimension each direction already controls, cycling 1/4 → 1/3 → 1/2 → 2/3. Never carries over to the next pick.")
                QQC2.ToolTip.visible: hovered
            }
        }
    }
}
