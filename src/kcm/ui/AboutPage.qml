import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// About: version, how to use it, and the keyd setup steps (moved here from
// the old warning banner).
FormPage {
    id: root

    readonly property string repoUrl: "https://github.com/strandzen/Sirkel"

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            level: 1
            text: i18n("Sirkel %1", kcm.version)
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Loop-inspired window snapping for KDE Plasma. Hold Caps Lock, move the mouse or press a direction key, release to snap the window there.")
        }

        Kirigami.Heading {
            level: 3
            text: i18n("Setting up the hyperkey")
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Sirkel listens for F24. keyd turns Caps Lock into F24. Add this to /etc/keyd/default.conf (Sirkel never writes there itself):")
        }

        QQC2.TextArea {
            Layout.fillWidth: true
            readOnly: true
            selectByMouse: true
            font.family: "monospace"
            text: "[ids]\n*\n\n[main]\ncapslock = f24"
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Then enable it once with: sudo systemctl enable --now keyd")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: kcm.keydOk ? Kirigami.MessageType.Positive : Kirigami.MessageType.Warning
            text: kcm.keydOk ? i18n("keyd is set up.") : i18n("keyd isn't set up yet. Reopen these settings after setting it up to check again.")
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Button {
                text: i18n("Project page")
                icon.name: "internet-services"
                onClicked: Qt.openUrlExternally(root.repoUrl)
            }
            QQC2.Button {
                text: i18n("Report a problem")
                icon.name: "tools-report-bug"
                onClicked: Qt.openUrlExternally(root.repoUrl + "/issues")
            }
        }
    }
}
