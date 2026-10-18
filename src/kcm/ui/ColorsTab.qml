import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Colors tab. Owns colorPopup (the custom hex/RGBA picker), since only this
// tab uses it.
QQC2.ScrollView {
    id: root

    property real fieldWidth: Kirigami.Units.gridUnit * 14
    property real formWidth: Kirigami.Units.gridUnit * 30

    contentWidth: availableWidth
    // Reserve the scrollbar's width permanently so overflowing doesn't
    // narrow the form into label-above-field mode.
    QQC2.ScrollBar.vertical.policy: QQC2.ScrollBar.AlwaysOn

    Item {
        width: parent.width
        implicitHeight: colorsForm.implicitHeight + Kirigami.Units.largeSpacing * 2

        Kirigami.FormLayout {
            id: colorsForm
            width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, root.formWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Kirigami.Units.largeSpacing

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Color source")
            }

            QQC2.Switch {
                Kirigami.FormData.label: i18n("Linked:")
                text: i18n("One color for outline, pointer, and ring")
                checked: kcm.settings.linkIndicatorColors
                onToggled: kcm.settings.linkIndicatorColors = checked
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Outline")
            }

            ColorSourceCombo {
                Kirigami.FormData.label: i18n("Source:")
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

            // Pointer and Ring rows only matter when not linked.
            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Pointer")
                visible: !kcm.settings.linkIndicatorColors
            }

            ColorSourceCombo {
                Kirigami.FormData.label: i18n("Source:")
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

            // No wallpaper option: the ring is meant to keep its own
            // neutral identity.
            ColorSourceCombo {
                Kirigami.FormData.label: i18n("Source:")
                Layout.preferredWidth: root.fieldWidth
                visible: !kcm.settings.linkIndicatorColors
                sourceOptions: [
                    { value: "manual", text: i18n("Manual") },
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

            QQC2.TextField {
                Kirigami.FormData.label: i18n("Wallpaper color file:")
                placeholderText: i18n("Leave empty to use pywal's default output")
                text: kcm.settings.wallpaperColorPath
                onEditingFinished: kcm.settings.wallpaperColorPath = text

                QQC2.ToolTip.text: i18n("Used when a source above is \"From wallpaper\". Empty = pywal's default output. A path = a plain hex color file (matugen, etc.).")
                QQC2.ToolTip.visible: hovered
            }

            QQC2.Button {
                Kirigami.FormData.label: i18n("Reset:")
                text: i18n("Reset colors to defaults")
                onClicked: kcm.resetColors()

                QQC2.ToolTip.text: i18n("Resets just this tab. The Defaults button resets everything.")
                QQC2.ToolTip.visible: hovered
            }
        }
    }

    // A Popup rather than QtQuick.Dialogs' ColorDialog, whose platform
    // dialog didn't reliably appear from inside an embedded KCM.
    QQC2.Popup {
        id: colorPopup
        parent: root
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        width: Kirigami.Units.gridUnit * 18
        modal: true
        focus: true
        closePolicy: QQC2.Popup.CloseOnEscape | QQC2.Popup.CloseOnPressOutside

        // Set by ColorPickerRow right before open().
        property string target: "outline"
        property color initialColor

        property int red: 0
        property int green: 0
        property int blue: 0
        property int alpha: 255

        function commit() {
            kcm.pickColor(target, Qt.rgba(red / 255, green / 255, blue / 255, alpha / 255));
            hexField.text = toHex();
        }

        function toHex() {
            function h(v) { return v.toString(16).padStart(2, "0"); }
            return "#" + h(red) + h(green) + h(blue) + h(alpha);
        }

        function fromHex(text) {
            const m = /^#?([0-9a-fA-F]{6}([0-9a-fA-F]{2})?)$/.exec(text.trim());
            if (!m) {
                return false;
            }
            const hex = m[1];
            red = parseInt(hex.substring(0, 2), 16);
            green = parseInt(hex.substring(2, 4), 16);
            blue = parseInt(hex.substring(4, 6), 16);
            alpha = hex.length === 8 ? parseInt(hex.substring(6, 8), 16) : 255;
            return true;
        }

        onOpened: {
            red = Math.round(initialColor.r * 255);
            green = Math.round(initialColor.g * 255);
            blue = Math.round(initialColor.b * 255);
            alpha = Math.round(initialColor.a * 255);
            hexField.text = toHex();
        }

        // One R/G/B/A slider row.
        component ChannelRow: RowLayout {
            id: channel
            property string label
            property int channelValue
            signal picked(int value)
            Layout.fillWidth: true
            Text { text: channel.label; Layout.preferredWidth: Kirigami.Units.gridUnit; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }
            QQC2.Slider {
                Layout.fillWidth: true
                from: 0; to: 255; stepSize: 1
                value: channel.channelValue
                onMoved: channel.picked(Math.round(value))
            }
            Text { text: channel.channelValue; Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5; horizontalAlignment: Text.AlignRight; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                level: 4
                text: i18n("Custom color")
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                radius: 4
                border.width: 1
                border.color: Kirigami.Theme.textColor
                color: Qt.rgba(colorPopup.red / 255, colorPopup.green / 255, colorPopup.blue / 255, colorPopup.alpha / 255)
            }

            RowLayout {
                Layout.fillWidth: true

                Text { text: i18n("Hex:"); color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }

                QQC2.TextField {
                    id: hexField
                    Layout.fillWidth: true
                    placeholderText: "#RRGGBBAA"
                    onEditingFinished: {
                        if (colorPopup.fromHex(text)) {
                            colorPopup.commit();
                        } else {
                            text = colorPopup.toHex();
                        }
                    }
                }
            }

            ChannelRow { label: "R"; channelValue: colorPopup.red; onPicked: (v) => { colorPopup.red = v; colorPopup.commit(); } }
            ChannelRow { label: "G"; channelValue: colorPopup.green; onPicked: (v) => { colorPopup.green = v; colorPopup.commit(); } }
            ChannelRow { label: "B"; channelValue: colorPopup.blue; onPicked: (v) => { colorPopup.blue = v; colorPopup.commit(); } }
            ChannelRow { label: "A"; channelValue: colorPopup.alpha; onPicked: (v) => { colorPopup.alpha = v; colorPopup.commit(); } }

            QQC2.DialogButtonBox {
                Layout.fillWidth: true
                standardButtons: QQC2.DialogButtonBox.Close
                onRejected: colorPopup.close()
                onAccepted: colorPopup.close()
            }
        }
    }
}
