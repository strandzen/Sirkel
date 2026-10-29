// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

// Sirkel's settings: a sidebar of pages (one file each) next to the current
// page. Opens from Desktop Effects (gear icon) and as its own window from
// the "Sirkel" launcher entry. AbstractKCM because the settings window
// expects a Kirigami page as the root.
KCM.AbstractKCM {
    id: root
    implicitWidth: compactWidth
    implicitHeight: Kirigami.Units.gridUnit * 38
    framedView: false
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    // The page shown; settable so a page can be opened directly.
    property alias currentPage: sidebar.currentIndex
    property bool previewShown: true
    // Pages with a preview: Appearance and Colors.
    readonly property bool previewPage: currentPage === 1 || currentPage === 2
    readonly property bool previewActive: previewShown && previewPage
    readonly property int slideDuration: 220

    // The sidebar and the settings column have fixed widths, sized to their
    // content (FormPage's form width, its margins and the scrollbar). The
    // preview takes whatever width is left, so resizing the window by its
    // edge resizes only the preview.
    readonly property real sidebarWidth: Kirigami.Units.gridUnit * 10
    readonly property real settingsWidth: Kirigami.Units.gridUnit * 32
    readonly property real previewMinimumWidth: Kirigami.Units.gridUnit * 12
    // Width the preview gets when shown: its width when last hidden.
    property real previewWidth: Kirigami.Units.gridUnit * 46

    // Shown immediately on opening (it then grows with the window); kept
    // until the window has shrunk back on closing, so the settings column
    // doesn't stretch into the gap meanwhile.
    property bool paneVisible: previewActive
    // True while the window is sliding open or closed.
    property bool sliding: false
    Timer {
        id: slideTimer
        interval: root.slideDuration
        onTriggered: {
            root.sliding = false;
            if (!root.previewActive) {
                root.paneVisible = false;
            }
        }
    }

    // Sidebar + settings column + separators: the page without the preview.
    readonly property real compactWidth: sidebarWidth + settingsWidth + 2

    onPreviewActiveChanged: {
        if (previewActive) {
            paneVisible = true;
            kcm.resizeHostWindow(width, compactWidth + previewWidth, slideDuration);
        } else {
            // Remember the width only when settled, not mid-slide.
            if (!sliding && previewPane.width >= previewMinimumWidth) {
                previewWidth = previewPane.width;
            }
            kcm.resizeHostWindow(width, compactWidth, slideDuration);
        }
        sliding = true;
        slideTimer.restart();
    }

    // Show/Hide for the preview, in the window's title bar next to "Sirkel"
    // (the page header), on the pages that have one.
    actions: [
        Kirigami.Action {
            visible: root.previewPage
            text: root.previewShown ? i18n("Hide preview") : i18n("Show preview")
            icon.name: root.previewShown ? "sidebar-collapse-right" : "sidebar-expand-right"
            displayHint: Kirigami.DisplayHint.KeepVisible
            onTriggered: root.previewShown = !root.previewShown
        }
    ]

    readonly property var pages: [
        { name: i18n("General"), icon: "preferences-system-windows", source: "GeneralPage.qml" },
        { name: i18n("Appearance"), icon: "preferences-desktop-theme-global", source: "AppearancePage.qml" },
        { name: i18n("Colors"), icon: "preferences-desktop-color", source: "ColorsPage.qml" },
        { name: i18n("Shortcuts"), icon: "preferences-desktop-keyboard-shortcut", source: "ShortcutsPage.qml" },
        { name: i18n("About"), icon: "help-about", source: "AboutPage.qml" },
    ]

    // The rebuild notification opens the settings on the About page.
    function showRequestedPage() {
        if (kcm.requestedPage === "about") {
            currentPage = pages.findIndex(page => page.source === "AboutPage.qml");
        }
    }
    Component.onCompleted: showRequestedPage()
    Connections {
        target: kcm
        function onRequestedPageChanged() {
            root.showRequestedPage();
        }
    }

    // Back from a terminal (a rebuild) or keyd's config: check again, so
    // the status is current without reopening the settings. Only on the
    // pages that show it, as the window is also refocused by every dialog.
    Connections {
        target: root.Window.window
        function onActiveChanged() {
            const page = root.pages[root.currentPage].source;
            if (root.Window.window.active && (page === "GeneralPage.qml" || page === "AboutPage.qml")) {
                kcm.refreshStatus();
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        QQC2.ScrollView {
            Layout.fillHeight: true
            Layout.preferredWidth: root.sidebarWidth
            Layout.minimumWidth: root.sidebarWidth

            ListView {
                id: sidebar
                model: root.pages
                currentIndex: 0
                clip: true
                delegate: QQC2.ItemDelegate {
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    text: modelData.name
                    icon.name: modelData.icon
                    highlighted: ListView.isCurrentItem
                    onClicked: sidebar.currentIndex = index
                }
            }
        }

        Kirigami.Separator {
            Layout.fillHeight: true
        }

        // Fixed width while the preview is shown; fills the window otherwise.
        ColumnLayout {
            Layout.fillWidth: !previewPane.visible
            Layout.fillHeight: true
            Layout.preferredWidth: root.settingsWidth
            Layout.minimumWidth: root.settingsWidth
            spacing: 0

            // All pages are created up front (they're small): switching back
            // keeps their state, e.g. an expanded Advanced fold.
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: sidebar.currentIndex

                Repeater {
                    model: root.pages
                    delegate: Loader {
                        required property var modelData
                        source: modelData.source
                    }
                }
            }
        }

        Kirigami.Separator {
            Layout.fillHeight: true
            visible: previewPane.visible
        }

        // Live preview: takes all remaining width, so dragging the window's
        // edge resizes it. Opening it widens the window by its last width.
        PreviewPane {
            id: previewPane
            Layout.fillHeight: true
            Layout.fillWidth: true
            visible: root.paneVisible
        }
    }
}
