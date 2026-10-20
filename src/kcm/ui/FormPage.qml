import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Frame for one settings page: a scrollable column, capped to formWidth and
// centered. Pages put a page-specific header (if any), their main
// Kirigami.FormLayout and an AdvancedFold inside it.
QQC2.ScrollView {
    id: page

    // Shared so fields line up across pages.
    readonly property real fieldWidth: Kirigami.Units.gridUnit * 14
    readonly property real formWidth: Kirigami.Units.gridUnit * 30

    default property alias content: column.data

    contentWidth: availableWidth
    // Reserve the scrollbar's width permanently, so a page that overflows
    // doesn't lose width and wrap the form to label-above-field layout.
    QQC2.ScrollBar.vertical.policy: QQC2.ScrollBar.AlwaysOn

    Item {
        width: parent.width
        implicitHeight: column.implicitHeight + Kirigami.Units.largeSpacing * 2

        ColumnLayout {
            id: column
            width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, page.formWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing
        }
    }
}
