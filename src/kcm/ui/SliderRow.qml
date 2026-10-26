// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// A slider with its current value shown next to it, for settings tuned by
// eye. Set Kirigami.FormData.label on the instance like any form row.
// `moved` fires only for user changes, with the new value.
RowLayout {
    id: row

    property real from: 0
    property real to: 100
    property real stepSize: 1
    property real value: 0
    property string unit: ""
    property real fieldWidth: Kirigami.Units.gridUnit * 14

    signal moved(real value)

    function rounded(v) {
        return Math.round(v / row.stepSize) * row.stepSize;
    }

    Layout.preferredWidth: fieldWidth
    spacing: Kirigami.Units.smallSpacing

    QQC2.Slider {
        id: slider
        Layout.fillWidth: true
        from: row.from
        to: row.to
        // Moves freely and rounds when reporting: a real stepSize makes the
        // desktop style draw a tick mark for every step, a solid comb at 1 px.
        value: row.value
        onMoved: row.moved(row.rounded(value))
    }

    QQC2.Label {
        // Wide enough for the largest value, so the slider doesn't shift.
        Layout.preferredWidth: Kirigami.Units.gridUnit * 3
        horizontalAlignment: Text.AlignRight
        text: row.rounded(slider.value) + row.unit
    }
}
