// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// About: version, how to use it, whether Sirkel is built for the installed
// KWin, and the keyd setup steps until keyd is set up.
FormPage {
    id: root

    readonly property string repoUrl: "https://github.com/strandzen/Sirkel"

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            source: "io.github.strandzen.sirkel"
            implicitWidth: Kirigami.Units.iconSizes.huge * 1.5
            implicitHeight: Kirigami.Units.iconSizes.huge * 1.5
        }
        Kirigami.Heading {
            Layout.alignment: Qt.AlignHCenter
            level: 1
            text: i18n("Sirkel %1", kcm.version)
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Loop-inspired window snapping for KDE Plasma. Hold your trigger (set on the Shortcuts page), move the mouse or press a direction key, and let go to snap the window there.")
        }

        // KWin only loads Sirkel if it was built for the exact KWin version,
        // so every Plasma update (bugfix ones too) needs a rebuild.
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: kcm.effectStatus !== "checking"
            type: ({
                running: Kirigami.MessageType.Positive,
                rebuild: Kirigami.MessageType.Error,
                relogin: Kirigami.MessageType.Warning,
                disabled: Kirigami.MessageType.Information,
            })[kcm.effectStatus] ?? Kirigami.MessageType.Information
            text: ({
                running: i18n("Sirkel is built for KWin %1 and running.", kcm.kwinBuildVersion),
                rebuild: i18n("Sirkel is built for KWin %1, but KWin %2 is installed, so KWin won't load it. Rebuild and reinstall Sirkel, then log out and back in.", kcm.kwinBuildVersion, kcm.kwinInstalledVersion),
                relogin: i18n("Sirkel is built for KWin %1 but isn't running yet. Log out and back in.", kcm.kwinBuildVersion),
                disabled: i18n("Sirkel isn't active. Turn it on in System Settings → Desktop Effects."),
            })[kcm.effectStatus] ?? ""
        }

        // A package install carries its PKGBUILD, so it can rebuild from
        // here: sirkel-rebuild opens a terminal running makepkg -si.
        RowLayout {
            visible: kcm.effectStatus === "rebuild" && kcm.canRebuild && kcm.hasBuildTools
            spacing: Kirigami.Units.smallSpacing

            QQC2.Button {
                text: i18n("Rebuild for KWin %1", kcm.kwinInstalledVersion)
                icon.name: "run-build"
                onClicked: kcm.rebuild()
            }
            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: i18n("Opens a terminal that builds and installs Sirkel. You'll be asked for your password.")
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: kcm.effectStatus === "rebuild" && kcm.canRebuild && !kcm.hasBuildTools
            wrapMode: Text.WordWrap
            text: i18n("To rebuild Sirkel from here, first install the build tools: sudo pacman -S --needed base-devel")
        }

        QQC2.TextArea {
            Layout.fillWidth: true
            visible: kcm.effectStatus === "rebuild" && !kcm.canRebuild
            readOnly: true
            selectByMouse: true
            font.family: "monospace"
            text: i18n("# Built from source, in the source folder:\ncmake --build build\nsudo cmake --install build\n\n# Installed with the PKGBUILD, in its folder:\nmakepkg -si")
        }

        // The output of the last rebuild, to attach to a bug report if it
        // failed.
        QQC2.Button {
            visible: kcm.effectStatus === "rebuild" && kcm.rebuildLogUrl.toString() !== ""
            flat: true
            text: i18n("Show the last rebuild's output")
            icon.name: "text-x-log"
            onClicked: Qt.openUrlExternally(kcm.rebuildLogUrl)
        }

        // keyd: just a confirmation once it's set up, otherwise the steps.
        // Recommended even when a key combination is the trigger.
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: kcm.keydOk
            type: Kirigami.MessageType.Positive
            text: i18n("keyd is set up.")
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: !kcm.keydOk
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                level: 3
                text: i18n("Recommended: Caps Lock as the trigger")
            }

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: i18n("Sirkel listens for F24, and keyd can turn Caps Lock into F24. Add this to /etc/keyd/default.conf (Sirkel never writes there itself):")
            }

            QQC2.TextArea {
                id: keydConfig
                Layout.fillWidth: true
                readOnly: true
                selectByMouse: true
                font.family: "monospace"
                text: "[ids]\n*\n\n[main]\ncapslock = f24"
                rightPadding: copyButton.width + Kirigami.Units.smallSpacing * 2

                QQC2.ToolButton {
                    id: copyButton
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: Kirigami.Units.smallSpacing
                    icon.name: "edit-copy"
                    display: QQC2.AbstractButton.IconOnly
                    text: i18n("Copy")
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                    onClicked: {
                        keydConfig.selectAll();
                        keydConfig.copy();
                        keydConfig.deselect();
                    }
                }
            }

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                textFormat: Text.RichText
                text: i18n("Then enable keyd with <code>sudo systemctl enable --now keyd</code>, and choose Caps Lock on the Shortcuts page. Reopen these settings to check again.")
            }
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
