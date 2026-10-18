import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

// A color swatch plus "Customize…" button, shared by the Outline, Pointer and
// Ring rows. `colorValue` is bound statically by the caller (QML doesn't
// reliably track dynamic kcm[...] lookups inside bindings); `target` is only
// read imperatively when opening the popup.
RowLayout {
    id: root

    property color colorValue
    // "outline", "pointer" or "ring" (see SirkelConfigModule::pickColor).
    property string target
    property var popup

    spacing: Kirigami.Units.largeSpacing

    function openPopup() {
        root.popup.target = root.target;
        root.popup.initialColor = root.colorValue;
        root.popup.open();
    }

    // Checkerboard behind the swatch so transparency is visible.
    Item {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 3
        Layout.preferredHeight: Kirigami.Units.gridUnit * 2

        Grid {
            anchors.fill: parent
            columns: 6
            rows: 3
            Repeater {
                model: 18
                Rectangle {
                    width: parent.width / 6
                    height: parent.height / 3
                    color: (Math.floor(index / 6) + (index % 6)) % 2 === 0 ? "#cccccc" : "#ffffff"
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            color: root.colorValue
            border.width: 1
            border.color: Kirigami.Theme.textColor
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.openPopup()
        }
    }

    QQC2.Button {
        text: i18n("Customize…")
        onClicked: root.openPopup()
    }
}
