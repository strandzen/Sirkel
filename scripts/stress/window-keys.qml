// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// Logs every key it receives. Enter and Backspace must reach apps outside a
// Sirkel session (Sirkel's own Enter/Backspace shortcuts are inactive), and
// must not while a session holds the keyboard.
import QtQuick
import QtQuick.Window

Window {
    width: 400
    height: 300
    visible: true
    title: "sirkel-stress keys"
    color: "#777733"

    Item {
        anchors.fill: parent
        focus: true
        Keys.onPressed: (event) => console.log("SIRKEL-KEY", event.key)
    }
}
