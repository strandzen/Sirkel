// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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

        RowLayout {
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Icon {
                source: "io.github.strandzen.sirkel"
                implicitWidth: Kirigami.Units.iconSizes.huge
                implicitHeight: Kirigami.Units.iconSizes.huge
            }
            Kirigami.Heading {
                level: 1
                text: i18n("Sirkel %1", kcm.version)
            }
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
                running: i18n("Sirkel is running (built for KWin %1).", kcm.kwinBuildVersion),
                rebuild: i18n("Sirkel was built for KWin %1, but KWin %2 is installed, so KWin won't load it. Rebuild and reinstall Sirkel, then log out and back in.", kcm.kwinBuildVersion, kcm.kwinInstalledVersion),
                relogin: i18n("Sirkel matches KWin %1 but isn't running yet. Log out and back in.", kcm.kwinBuildVersion),
                disabled: i18n("Sirkel is off. Turn it on in System Settings → Desktop Effects."),
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

        Kirigami.Heading {
            level: 3
            text: i18n("Optional: Caps Lock as the trigger")
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Sirkel listens for F24, and keyd can turn Caps Lock into F24. Add this to /etc/keyd/default.conf (Sirkel never writes there itself):")
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
            text: i18n("Then enable it once with: sudo systemctl enable --now keyd, and choose Caps Lock on the Shortcuts page.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            // keyd only matters when Caps Lock is (or could become) the trigger.
            type: kcm.triggerKind === "combination" ? Kirigami.MessageType.Information
                : kcm.keydOk ? Kirigami.MessageType.Positive : Kirigami.MessageType.Warning
            text: kcm.triggerKind === "combination"
                ? i18n("keyd isn't needed: %1 is set as the trigger. Set it up only if you'd rather use Caps Lock.", kcm.triggerText)
                : kcm.keydOk ? i18n("keyd is set up.") : i18n("keyd isn't set up yet. Reopen these settings after setting it up to check again.")
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
