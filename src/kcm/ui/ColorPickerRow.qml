// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import org.kde.kquickcontrols as KQuickControls

// KDE's standard color button for the Outline, Pointer and Ring rows: a
// swatch that opens Plasma's color dialog, alpha included. `colorValue` is
// bound statically by the caller (QML doesn't reliably track dynamic
// kcm[...] lookups inside bindings); `target` names the color for
// SirkelConfigModule::pickColor().
KQuickControls.ColorButton {
    id: root

    property color colorValue
    // "outline", "pointer" or "ring".
    property string target

    // `color` is an alias for the dialog's selected color, which the dialog
    // overwrites when a color is picked, so a binding wouldn't survive.
    // Pushed explicitly instead, e.g. after "Reset colors" or an accent change.
    onColorValueChanged: color = colorValue
    Component.onCompleted: color = colorValue
    showAlphaChannel: true
    dialogTitle: i18n("Choose a color")
    onAccepted: (picked) => kcm.pickColor(root.target, picked)
}
