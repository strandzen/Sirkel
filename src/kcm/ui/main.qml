import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Thin shell wiring the three tabs (one file each) into a tab bar:
// Input (how you trigger and control it), Visuals (how it looks and
// moves), Colors (what color it is).
ColumnLayout {
    id: root
    implicitWidth: Kirigami.Units.gridUnit * 32
    implicitHeight: Kirigami.Units.gridUnit * 26
    spacing: 0

    // Shared by every tab so fields line up across tabs: one field width,
    // and each form capped to formWidth and centered.
    readonly property real fieldWidth: Kirigami.Units.gridUnit * 14
    readonly property real formWidth: Kirigami.Units.gridUnit * 30

    Kirigami.Heading {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.smallSpacing
        level: 1
        text: i18n("Settings — Sirkel")
    }

    QQC2.TabBar {
        id: tabBar
        Layout.fillWidth: true

        // Spread the tab buttons evenly across the bar.
        QQC2.TabButton { text: i18n("Input"); width: tabBar.width / tabBar.count }
        QQC2.TabButton { text: i18n("Visuals"); width: tabBar.width / tabBar.count }
        QQC2.TabButton { text: i18n("Colors"); width: tabBar.width / tabBar.count }
    }

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabBar.currentIndex

        InputTab { fieldWidth: root.fieldWidth; formWidth: root.formWidth }
        VisualsTab { fieldWidth: root.fieldWidth; formWidth: root.formWidth }
        ColorsTab { fieldWidth: root.fieldWidth; formWidth: root.formWidth }
    }
}
