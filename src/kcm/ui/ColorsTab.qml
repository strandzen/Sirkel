import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Colors tab content, split out of main.qml — see InputTab.qml's header
// comment for why tabs are split files, and for the ScrollBar.AlwaysOn
// fix below. Owns colorPopup itself (the custom hex/RGBA picker), since
// it's only ever used from here.
QQC2.ScrollView {
    id: root

    property real fieldWidth: Kirigami.Units.gridUnit * 14
    property real formWidth: Kirigami.Units.gridUnit * 30

    contentWidth: availableWidth
    QQC2.ScrollBar.vertical.policy: QQC2.ScrollBar.AlwaysOn

    Item {
        width: parent.width
        implicitHeight: colorsForm.implicitHeight + Kirigami.Units.largeSpacing * 2

        Kirigami.FormLayout {
            id: colorsForm
            width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, root.formWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Kirigami.Units.largeSpacing

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Color source"
            }

            // Governs all three groups below at once, so it stays here,
            // outside/above all of them, rather than nested under any one
            // group's own section.
            QQC2.Switch {
                Kirigami.FormData.label: "Linked:"
                text: "One color for outline, pointer, and ring"
                checked: kcm.linkIndicatorColors
                onToggled: kcm.linkIndicatorColors = checked
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Outline"
            }

            // Outline — always shown. ColorPickerRow/ColorSourceCombo (see
            // those files) take kcm.* values in statically (not via
            // dynamic/bracket-notation property lookups): QML's binding
            // engine doesn't reliably track dependencies through those
            // inside declarative bindings, confirmed the hard way earlier
            // this project's history (a ListView model that silently
            // rendered nothing). Imperative one-shot reads/writes (inside
            // functions, not bindings) don't have that problem, so
            // colorPopup below still uses a dynamic target safely.
            ColorSourceCombo {
                Kirigami.FormData.label: "Source:"
                Layout.preferredWidth: root.fieldWidth
                sourceValue: kcm.outlineColorSource
                onSourcePicked: (value) => kcm.outlineColorSource = value
            }

            ColorPickerRow {
                Kirigami.FormData.label: "Color:"
                colorValue: kcm.outlineColor
                target: "outlineColor"
                popup: colorPopup
            }

            // Pointer/Ring groups (including their own section headers) —
            // only shown when not linked to the outline's. Kirigami.
            // FormLayout collapses hidden rows (no blank gap left behind),
            // same pattern as every other conditional row already in this
            // KCM.
            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Pointer"
                visible: !kcm.linkIndicatorColors
            }

            ColorSourceCombo {
                Kirigami.FormData.label: "Source:"
                Layout.preferredWidth: root.fieldWidth
                visible: !kcm.linkIndicatorColors
                sourceValue: kcm.indicatorPointerColorSource
                onSourcePicked: (value) => kcm.indicatorPointerColorSource = value
            }

            ColorPickerRow {
                Kirigami.FormData.label: "Color:"
                visible: !kcm.linkIndicatorColors
                colorValue: kcm.indicatorPointerColor
                target: "indicatorPointerColor"
                popup: colorPopup
            }

            // IndicatorRingColorSource defaults to "manual" (not "theme"),
            // so the ring keeps its own distinct look out of the box even
            // once unlinked - and its own source list deliberately drops
            // "wallpaper": the ring's whole point is to hold its own
            // distinct identity (see LinkIndicatorColors defaulting to
            // false), which a live-tracked wallpaper color works against.
            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Ring"
                visible: !kcm.linkIndicatorColors
            }

            ColorSourceCombo {
                Kirigami.FormData.label: "Source:"
                Layout.preferredWidth: root.fieldWidth
                visible: !kcm.linkIndicatorColors
                sourceOptions: [
                    { value: "manual", text: "Manual" },
                    { value: "theme", text: "Theme accent (live)" },
                ]
                sourceValue: kcm.indicatorRingColorSource
                onSourcePicked: (value) => kcm.indicatorRingColorSource = value
            }

            ColorPickerRow {
                Kirigami.FormData.label: "Color:"
                visible: !kcm.linkIndicatorColors
                colorValue: kcm.indicatorRingColor
                target: "indicatorRingColor"
                popup: colorPopup
            }

            QQC2.TextField {
                Kirigami.FormData.label: "Wallpaper color file:"
                placeholderText: "Leave empty to use pywal's default output"
                text: kcm.wallpaperColorPath
                onEditingFinished: kcm.wallpaperColorPath = text

                QQC2.ToolTip.text: "Used when a source above is \"From wallpaper\". Empty = pywal's default output. A path = a plain hex color file (matugen, etc.)."
                QQC2.ToolTip.visible: hovered
            }

            QQC2.Button {
                Kirigami.FormData.label: "Reset:"
                text: "Reset colors to defaults"
                onClicked: kcm.resetColors()

                QQC2.ToolTip.text: "Resets just this tab. The KCM's own Defaults button resets everything."
                QQC2.ToolTip.visible: hovered
            }

            // The "theme colors quick-pick" list that used to live here
            // was removed — it never rendered its text correctly across
            // several different rewrites (RowLayout, raw x/y
            // positioning, QQC2.Label, plain Text, explicit colors; see
            // timeline.md for the full trail), and "Theme accent (live)"
            // in the source dropdowns above already covers "use a theme
            // color" without it. Sticking to source + manual picker for
            // now.
        }
    }

    // Custom color picker — deliberately not QtQuick.Dialogs' ColorDialog,
    // which relies on a native/platform dialog backend that didn't reliably
    // show up at all when opened from inside an embedded KCM view. A Popup
    // renders directly inside this page's own window, so it always works
    // regardless of platform dialog support.
    QQC2.Popup {
        id: colorPopup
        parent: root
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        width: Kirigami.Units.gridUnit * 18
        modal: true
        focus: true
        closePolicy: QQC2.Popup.CloseOnEscape | QQC2.Popup.CloseOnPressOutside

        // Which kcm color property this instance is currently editing —
        // set right before open() by whichever "Customize…" button was
        // clicked (inside a ColorPickerRow). Read/written only inside the
        // plain JS functions below, never as part of a declarative
        // binding, so the kcm[...] dynamic lookup here is safe (see the
        // note above the Outline ComboBox).
        property string targetProperty: "outlineColor"

        property int red: 0
        property int green: 0
        property int blue: 0
        property int alpha: 255

        function syncFromConfig() {
            const current = kcm[targetProperty];
            red = Math.round(current.r * 255);
            green = Math.round(current.g * 255);
            blue = Math.round(current.b * 255);
            alpha = Math.round(current.a * 255);
            hexField.text = toHex();
        }

        function commit() {
            kcm[targetProperty] = Qt.rgba(red / 255, green / 255, blue / 255, alpha / 255);
        }

        function toHex() {
            function h(v) { return v.toString(16).padStart(2, "0"); }
            return "#" + h(red) + h(green) + h(blue) + h(alpha);
        }

        function fromHex(text) {
            const m = /^#?([0-9a-fA-F]{6}([0-9a-fA-F]{2})?)$/.exec(text.trim());
            if (!m) {
                return false;
            }
            const hex = m[1];
            red = parseInt(hex.substring(0, 2), 16);
            green = parseInt(hex.substring(2, 4), 16);
            blue = parseInt(hex.substring(4, 6), 16);
            alpha = hex.length === 8 ? parseInt(hex.substring(6, 8), 16) : 255;
            return true;
        }

        onOpened: syncFromConfig()

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                level: 4
                text: "Custom color"
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                radius: 4
                border.width: 1
                border.color: Kirigami.Theme.textColor
                color: Qt.rgba(colorPopup.red / 255, colorPopup.green / 255, colorPopup.blue / 255, colorPopup.alpha / 255)
            }

            RowLayout {
                Layout.fillWidth: true

                Text { text: "Hex:"; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }

                QQC2.TextField {
                    id: hexField
                    Layout.fillWidth: true
                    placeholderText: "#RRGGBBAA"
                    onEditingFinished: {
                        if (colorPopup.fromHex(text)) {
                            colorPopup.commit();
                        } else {
                            text = colorPopup.toHex();
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Text { text: "R"; Layout.preferredWidth: Kirigami.Units.gridUnit; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }
                QQC2.Slider {
                    Layout.fillWidth: true
                    from: 0; to: 255; stepSize: 1
                    value: colorPopup.red
                    onMoved: {
                        colorPopup.red = Math.round(value);
                        colorPopup.commit();
                        hexField.text = colorPopup.toHex();
                    }
                }
                Text { text: colorPopup.red; Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5; horizontalAlignment: Text.AlignRight; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }
            }

            RowLayout {
                Layout.fillWidth: true
                Text { text: "G"; Layout.preferredWidth: Kirigami.Units.gridUnit; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }
                QQC2.Slider {
                    Layout.fillWidth: true
                    from: 0; to: 255; stepSize: 1
                    value: colorPopup.green
                    onMoved: {
                        colorPopup.green = Math.round(value);
                        colorPopup.commit();
                        hexField.text = colorPopup.toHex();
                    }
                }
                Text { text: colorPopup.green; Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5; horizontalAlignment: Text.AlignRight; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }
            }

            RowLayout {
                Layout.fillWidth: true
                Text { text: "B"; Layout.preferredWidth: Kirigami.Units.gridUnit; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }
                QQC2.Slider {
                    Layout.fillWidth: true
                    from: 0; to: 255; stepSize: 1
                    value: colorPopup.blue
                    onMoved: {
                        colorPopup.blue = Math.round(value);
                        colorPopup.commit();
                        hexField.text = colorPopup.toHex();
                    }
                }
                Text { text: colorPopup.blue; Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5; horizontalAlignment: Text.AlignRight; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }
            }

            RowLayout {
                Layout.fillWidth: true
                Text { text: "A"; Layout.preferredWidth: Kirigami.Units.gridUnit; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }
                QQC2.Slider {
                    Layout.fillWidth: true
                    from: 0; to: 255; stepSize: 1
                    value: colorPopup.alpha
                    onMoved: {
                        colorPopup.alpha = Math.round(value);
                        colorPopup.commit();
                        hexField.text = colorPopup.toHex();
                    }
                }
                Text { text: colorPopup.alpha; Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5; horizontalAlignment: Text.AlignRight; color: Kirigami.Theme.textColor; verticalAlignment: Text.AlignVCenter }
            }

            QQC2.DialogButtonBox {
                Layout.fillWidth: true
                standardButtons: QQC2.DialogButtonBox.Close
                onRejected: colorPopup.close()
                onAccepted: colorPopup.close()
            }
        }
    }
}
