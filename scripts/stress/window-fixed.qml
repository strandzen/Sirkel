// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// A window that can't be resized: Sirkel moves it without resizing.
import QtQuick
import QtQuick.Window

Window {
    width: 400
    height: 300
    minimumWidth: 400
    maximumWidth: 400
    minimumHeight: 300
    maximumHeight: 300
    visible: true
    title: "sirkel-stress fixed"
    color: "#775533"
}
