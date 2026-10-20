import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// "▸ Advanced": the less common settings of a page, folded away by default.
// Its own Kirigami.FormLayout, label column aligned with the page's main
// form through twinFormLayouts.
ColumnLayout {
    id: fold

    property string title: i18n("Advanced")
    // The page's main form, so both label columns share one width.
    property var twin: null
    property bool expanded: false

    default property alias content: form.data

    Layout.fillWidth: true
    spacing: Kirigami.Units.smallSpacing

    QQC2.ToolButton {
        Layout.alignment: Qt.AlignHCenter
        text: fold.title
        icon.name: fold.expanded ? "arrow-down" : "arrow-right"
        onClicked: fold.expanded = !fold.expanded
    }

    Kirigami.FormLayout {
        id: form
        Layout.fillWidth: true
        visible: fold.expanded
        twinFormLayouts: fold.twin ? [fold.twin] : []
    }
}
