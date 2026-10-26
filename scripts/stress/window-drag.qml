// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// A window that gets snapped and then dragged away: it must get its size
// back. A title of its own so the probe can find it.
import QtQuick
import QtQuick.Window

Window {
    width: 640
    height: 480
    visible: true
    title: "sirkel-stress drag"
    color: "#337755"
}
