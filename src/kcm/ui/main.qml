import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Tabbed instead of one long scrolling page — with every setting added over
// time, a single FormLayout no longer fit the dialog's default size at all
// (confirmed live: things were being silently clipped). Three tabs, each
// answering one question about a setting rather than "which visual element
// is this": Input (how you trigger/control it), Visuals (what it looks
// like and how it moves — Indicator and Outline sections, plus a shared
// Motion section for the two settings that already applied to both),
// Colors (what color it is). Each tab's content lives in its own file
// (InputTab.qml / VisualsTab.qml / ColorsTab.qml) — this file is just the
// thin shell that wires them into the tab bar, kept that way once the
// single-file version pushed past 900 lines. fieldWidth/formWidth are
// handed down to each tab so every field across every tab stays visually
// aligned; ColorPickerRow and ColorSourceCombo (used only by
// ColorsTab.qml) are shared reusable pieces for the Outline/Pointer/Ring
// color pickers, which used to be ~60 lines of near-identical QML
// repeated three times.
ColumnLayout {
    id: root
    implicitWidth: Kirigami.Units.gridUnit * 32
    implicitHeight: Kirigami.Units.gridUnit * 26
    spacing: 0

    // Shared across every tab so the page reads as one consistent design
    // instead of each tab doing its own thing: every single-value field
    // (SpinBox/ComboBox/TextField) is the same width, and each tab's whole
    // form is capped to this width and centered rather than stretching
    // edge-to-edge in a resized window.
    readonly property real fieldWidth: Kirigami.Units.gridUnit * 14
    readonly property real formWidth: Kirigami.Units.gridUnit * 30

    Kirigami.Heading {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.smallSpacing
        level: 1
        text: "Settings — Sirkel"
    }

    QQC2.TabBar {
        id: tabBar
        Layout.fillWidth: true

        // TabBar doesn't stretch its buttons to fill the bar on its own —
        // each one just sizes to its own text, left-aligned, leaving the
        // rest of the bar blank. Dividing the bar's width evenly spans it.
        QQC2.TabButton { text: "Input"; width: tabBar.width / tabBar.count }
        QQC2.TabButton { text: "Visuals"; width: tabBar.width / tabBar.count }
        QQC2.TabButton { text: "Colors"; width: tabBar.width / tabBar.count }
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
