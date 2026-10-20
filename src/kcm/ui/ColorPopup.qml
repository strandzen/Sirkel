import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Custom hex/RGBA color picker. A Popup rather than QtQuick.Dialogs'
// ColorDialog, whose platform dialog didn't reliably appear from inside an
// embedded KCM. Opened by ColorPickerRow, which sets `target` and
// `initialColor`; every change is applied immediately via kcm.pickColor().
QQC2.Popup {
    id: colorPopup
    // Centered in whatever it's parented to (set by the page).
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
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
