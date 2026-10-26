// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// A window wider than half the test screen can hold (1600 px): snapping it
// to a half must keep it on screen at its minimum width.
import QtQuick
import QtQuick.Window

Window {
    width: 1100
    height: 500
    minimumWidth: 1000
    visible: true
    title: "sirkel-stress minsize"
    color: "#553377"
}
