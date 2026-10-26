// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// A fullscreen window: Sirkel must leave it alone.
import QtQuick
import QtQuick.Window

Window {
    visibility: Window.FullScreen
    visible: true
    title: "sirkel-stress fullscreen"
    color: "#557733"
}
