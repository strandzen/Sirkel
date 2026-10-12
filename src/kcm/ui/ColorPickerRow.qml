import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

// One color swatch + "Customize…" row, reused for the Outline/Pointer/Ring
// color pickers in the Colors tab — these used to be ~60 lines of near-
// identical QML each (184 lines total across the three, confirmed by
// audit). `target` names which kcm color property this instance edits
// (e.g. "outlineColor") and `colorValue` is that property's actual value,
// passed in statically by the caller (kcm.outlineColor, a plain named
// binding) rather than looked up dynamically in here — QML's binding
// dependency tracking doesn't reliably follow dynamic/bracket-notation
// property lookups inside declarative bindings (confirmed the hard way
// earlier in this project's history with a ListView that silently
// rendered nothing). `target` itself is only ever read inside the
// imperative onClicked handlers below, which don't have that problem.
RowLayout {
    id: root

    property color colorValue
    property string target
    property var popup

    spacing: Kirigami.Units.largeSpacing

    // A checkerboard behind the swatch so a transparent/semi-transparent
    // color is actually visible, not just washed out.
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
            onClicked: {
                root.popup.targetProperty = root.target;
                root.popup.open();
            }
        }
    }

    QQC2.Button {
        text: "Customize…"
        onClicked: {
            root.popup.targetProperty = root.target;
            root.popup.open();
        }
    }
}
