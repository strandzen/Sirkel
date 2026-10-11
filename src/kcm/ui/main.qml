import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Tabbed instead of one long scrolling page — with every setting added over
// time (indicator, outline, colors, keybinds...) a single FormLayout no
// longer fit the dialog's default size at all (confirmed live: everything
// past "Indicator text" was being silently clipped). Each tab's own content
// is still wrapped in a ScrollView in case a future addition overflows a
// single tab too.
ColumnLayout {
    id: root
    implicitWidth: Kirigami.Units.gridUnit * 32
    implicitHeight: Kirigami.Units.gridUnit * 26
    spacing: 0

    // Shared across every tab so the page reads as one consistent design
    // instead of each tab doing its own thing: every single-value field
    // (SpinBox/ComboBox/TextField) is the same width, and each tab's whole
    // form is capped to this width and centered rather than stretching
    // edge-to-edge in a resized window.
    readonly property real fieldWidth: Kirigami.Units.gridUnit * 14
    readonly property real formWidth: Kirigami.Units.gridUnit * 30

    Kirigami.Heading {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.smallSpacing
        level: 1
        text: "Settings — Sirkel"
    }

    QQC2.TabBar {
        id: tabBar
        Layout.fillWidth: true

        // TabBar doesn't stretch its buttons to fill the bar on its own —
        // each one just sizes to its own text, left-aligned, leaving the
        // rest of the bar blank. Dividing the bar's width evenly spans it.
        QQC2.TabButton { text: "Behavior"; width: tabBar.width / tabBar.count }
        QQC2.TabButton { text: "Indicator"; width: tabBar.width / tabBar.count }
        QQC2.TabButton { text: "Outline"; width: tabBar.width / tabBar.count }
        QQC2.TabButton { text: "Colors"; width: tabBar.width / tabBar.count }
    }

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabBar.currentIndex

        // --- Behavior -------------------------------------------------
        QQC2.ScrollView {
            contentWidth: availableWidth

            Item {
                width: parent.width
                implicitHeight: behaviorForm.implicitHeight + Kirigami.Units.largeSpacing * 2

                Kirigami.FormLayout {
                    id: behaviorForm
                    width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, root.formWidth)
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: Kirigami.Units.largeSpacing

                    Kirigami.Separator {
                        Kirigami.FormData.isSection: true
                        Kirigami.FormData.label: "Mouse movement deadzone"
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Horizontal (px):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 0
                        to: 500
                        value: kcm.horizontalDeadzone
                        onValueModified: kcm.horizontalDeadzone = value
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Vertical (px):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 0
                        to: 500
                        value: kcm.verticalDeadzone
                        onValueModified: kcm.verticalDeadzone = value
                    }

                    Kirigami.Separator {
                        Kirigami.FormData.isSection: true
                        Kirigami.FormData.label: "Padding"
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Horizontal (px):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 0
                        to: 200
                        value: kcm.paddingHorizontal
                        onValueModified: kcm.paddingHorizontal = value
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Vertical (px):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 0
                        to: 200
                        value: kcm.paddingVertical
                        onValueModified: kcm.paddingVertical = value
                    }

                    Kirigami.Separator {
                        Kirigami.FormData.isSection: true
                        Kirigami.FormData.label: "Monitors"
                    }

                    QQC2.Switch {
                        Kirigami.FormData.label: "Targeting:"
                        text: "Snap to the cursor's monitor instead of the window's own"
                        checked: kcm.snapToCursorScreen
                        onToggled: kcm.snapToCursorScreen = checked

                        QQC2.ToolTip.text: "Decided once when you press the hyperkey, from wherever the cursor is at that moment — not tracked live if you drag across monitors mid-gesture."
                        QQC2.ToolTip.visible: hovered
                    }

                    Kirigami.Separator {
                        Kirigami.FormData.isSection: true
                        Kirigami.FormData.label: "Keybinds"
                    }

                    QQC2.ComboBox {
                        Kirigami.FormData.label: "Movement keys:"
                        Layout.preferredWidth: root.fieldWidth
                        textRole: "text"
                        // "none" disables direction-key bindings entirely
                        // for users who only want to drag the mouse — see
                        // axisForArmedKey() in sirkeleffect.cpp.
                        model: kcm.keybindSchemes.map(scheme => ({
                            value: scheme,
                            text: ({ arrows: "Arrows", wasd: "WASD", hjkl: "HJKL", none: "None (mouse only)" })[scheme] || scheme
                        }))
                        currentIndex: kcm.keybindSchemes.indexOf(kcm.keybindScheme)
                        onActivated: kcm.keybindScheme = kcm.keybindSchemes[currentIndex]
                    }

                    QQC2.ComboBox {
                        Kirigami.FormData.label: "Hyperkey trigger:"
                        Layout.preferredWidth: root.fieldWidth
                        textRole: "text"
                        // "delay" and "doubleclick" both need to watch for
                        // the key's release before committing to a session
                        // — see TriggerPhase in sirkeleffect.h.
                        model: [
                            { value: "instant", text: "Instant" },
                            { value: "delay", text: "Delay" },
                            { value: "doubleclick", text: "Double-click" },
                        ]
                        currentIndex: model.findIndex(m => m.value === kcm.triggerMode)
                        onActivated: kcm.triggerMode = model[currentIndex].value

                        QQC2.ToolTip.text: "Instant: arms as soon as you press it, like today. Delay: must be held for the duration below before it arms — releasing sooner cancels. Double-click: a quick tap then a second press-and-hold (within about 0.4s) arms it; a single press-and-hold does nothing."
                        QQC2.ToolTip.visible: hovered
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Delay (ms):"
                        Layout.preferredWidth: root.fieldWidth
                        visible: kcm.triggerMode === "delay"
                        from: 0
                        to: 1000
                        stepSize: 50
                        value: kcm.triggerDelay
                        onValueModified: kcm.triggerDelay = value
                    }

                    Kirigami.Separator {
                        Kirigami.FormData.isSection: true
                        Kirigami.FormData.label: "Scroll sizing"
                    }

                    QQC2.Switch {
                        Kirigami.FormData.label: "While previewing:"
                        text: "Scroll to resize before releasing"
                        checked: kcm.scrollResizeEnabled
                        onToggled: kcm.scrollResizeEnabled = checked

                        QQC2.ToolTip.text: "Left/right/top/bottom: resizes only the one dimension that direction already controls (e.g. right only changes width). Maximize: shrinks width only, centered horizontally — height stays full. Corners are unaffected. Never carries over — picking a new direction always starts back at the normal size."
                        QQC2.ToolTip.visible: hovered
                    }

                    QQC2.SpinBox {
                        // One decimal place matters here — see
                        // ScrollResizeSmallPercent's kcfg doc comment (three
                        // 33% windows side by side leave a visible gap;
                        // 33.3% doesn't). QQC2.SpinBox is integer-only, so
                        // this one counts in tenths of a percent internally
                        // and only formats/parses as a decimal percentage.
                        Kirigami.FormData.label: "Smaller size (%):"
                        Layout.preferredWidth: root.fieldWidth
                        visible: kcm.scrollResizeEnabled
                        from: 100
                        to: 499
                        value: Math.round(kcm.scrollResizeSmallPercent * 10)
                        onValueModified: kcm.scrollResizeSmallPercent = value / 10
                        textFromValue: (value) => (value / 10).toFixed(1)
                        valueFromText: (text) => Math.round(parseFloat(text) * 10)
                        validator: DoubleValidator {
                            bottom: 10.0
                            top: 49.9
                            decimals: 1
                            notation: DoubleValidator.StandardNotation
                        }

                        QQC2.ToolTip.text: "The smallest scroll step for edges, and Maximize's smallest (still centered) step."
                        QQC2.ToolTip.visible: hovered
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Larger size (%):"
                        Layout.preferredWidth: root.fieldWidth
                        visible: kcm.scrollResizeEnabled
                        from: 501
                        to: 900
                        value: Math.round(kcm.scrollResizeLargePercent * 10)
                        onValueModified: kcm.scrollResizeLargePercent = value / 10
                        textFromValue: (value) => (value / 10).toFixed(1)
                        valueFromText: (text) => Math.round(parseFloat(text) * 10)
                        validator: DoubleValidator {
                            bottom: 50.1
                            top: 90.0
                            decimals: 1
                            notation: DoubleValidator.StandardNotation
                        }

                        QQC2.ToolTip.text: "The largest scroll step for edges only — Maximize has no equivalent, since its own default is already full size."
                        QQC2.ToolTip.visible: hovered
                    }
                }
            }
        }

        // --- Indicator --------------------------------------------------
        QQC2.ScrollView {
            contentWidth: availableWidth

            Item {
                width: parent.width
                implicitHeight: indicatorForm.implicitHeight + Kirigami.Units.largeSpacing * 2

                Kirigami.FormLayout {
                    id: indicatorForm
                    width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, root.formWidth)
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: Kirigami.Units.largeSpacing

                    QQC2.Switch {
                        Kirigami.FormData.label: "Visible:"
                        text: "Show the indicator"
                        checked: kcm.showIndicator
                        onToggled: kcm.showIndicator = checked
                    }

                    QQC2.Switch {
                        Kirigami.FormData.label: "Text:"
                        text: "Show direction name in indicator"
                        checked: kcm.showIndicatorText
                        onToggled: kcm.showIndicatorText = checked
                    }

                    QQC2.Switch {
                        Kirigami.FormData.label: "Position:"
                        text: "Spawn at cursor instead of screen center"
                        checked: kcm.indicatorFollowsMouse
                        onToggled: kcm.indicatorFollowsMouse = checked
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Size (px):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 60
                        to: 400
                        value: kcm.indicatorSize
                        onValueModified: kcm.indicatorSize = value
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Corner radius (px):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 0
                        to: kcm.indicatorSize / 2
                        value: kcm.indicatorCornerRadius
                        onValueModified: kcm.indicatorCornerRadius = value
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Line thickness (px):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 2
                        to: 60
                        value: kcm.indicatorRingWidth
                        onValueModified: kcm.indicatorRingWidth = value
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Pointer length (px):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 6
                        to: 200
                        value: kcm.indicatorPointerLength
                        onValueModified: kcm.indicatorPointerLength = value
                    }
                }
            }
        }

        // --- Outline ------------------------------------------------------
        QQC2.ScrollView {
            contentWidth: availableWidth

            Item {
                width: parent.width
                implicitHeight: outlineForm.implicitHeight + Kirigami.Units.largeSpacing * 2

                Kirigami.FormLayout {
                    id: outlineForm
                    width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, root.formWidth)
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: Kirigami.Units.largeSpacing

                    QQC2.Switch {
                        Kirigami.FormData.label: "Visible:"
                        text: "Show the window placement preview"
                        checked: kcm.showOutline
                        onToggled: kcm.showOutline = checked
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Border thickness (px):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 1
                        to: 20
                        value: kcm.outlineBorderWidth
                        onValueModified: kcm.outlineBorderWidth = value
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Corner radius (px):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 0
                        to: 60
                        value: kcm.outlineCornerRadius
                        onValueModified: kcm.outlineCornerRadius = value
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Fill opacity (%):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 0
                        to: 100
                        value: kcm.outlineFillOpacity
                        onValueModified: kcm.outlineFillOpacity = value
                    }

                    QQC2.Switch {
                        Kirigami.FormData.label: "Blur:"
                        text: "Blur the desktop behind the fill"
                        checked: kcm.outlineBlur
                        onToggled: kcm.outlineBlur = checked

                        QQC2.ToolTip.text: "Uses KWin's own Blur effect — does nothing if you don't have Blur enabled in Desktop Effects. There's no separate blur strength here: that's a single compositor-wide setting KWin doesn't expose per-window, so Fill opacity above is what controls how much of the blur actually shows through once this is on."
                        QQC2.ToolTip.visible: hovered
                    }

                    QQC2.SpinBox {
                        Kirigami.FormData.label: "Animation duration (ms):"
                        Layout.preferredWidth: root.fieldWidth
                        from: 0
                        to: 1000
                        stepSize: 10
                        value: kcm.outlineAnimationDuration
                        onValueModified: kcm.outlineAnimationDuration = value

                        QQC2.ToolTip.text: "0 = instant (snaps straight to the new position/size). Higher values glide/grow the outline, and grow/shrink the indicator pointer into and out of maximize, more slowly."
                        QQC2.ToolTip.visible: hovered
                    }
                }
            }
        }

        // --- Colors ---------------------------------------------------
        QQC2.ScrollView {
            contentWidth: availableWidth

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

                QQC2.Switch {
                    Kirigami.FormData.label: "Linked:"
                    text: "Use the same color for both the outline and the indicator"
                    checked: kcm.linkIndicatorOutlineColor
                    onToggled: kcm.linkIndicatorOutlineColor = checked
                }

                // Outline color — always shown. Statically bound to
                // kcm.outlineColor/outlineColorSource throughout (not via
                // dynamic/bracket-notation property lookups): QML's binding
                // engine doesn't reliably track dependencies through those
                // inside declarative bindings, confirmed the hard way
                // earlier this session (a ListView model that silently
                // rendered nothing). Imperative one-shot reads/writes
                // (inside functions, not bindings) don't have that problem,
                // so colorPopup below still uses a dynamic target safely.
                QQC2.ComboBox {
                    Kirigami.FormData.label: "Outline source:"
                    Layout.preferredWidth: root.fieldWidth
                    textRole: "text"
                    model: [
                        { value: "manual", text: "Manual" },
                        { value: "theme", text: "Theme accent (live)" },
                        { value: "wallpaper", text: "From wallpaper (live)" },
                    ]
                    currentIndex: model.findIndex(m => m.value === kcm.outlineColorSource)
                    onActivated: kcm.outlineColorSource = model[currentIndex].value
                }

                RowLayout {
                    Kirigami.FormData.label: "Outline color:"
                    spacing: Kirigami.Units.largeSpacing

                    // A checkerboard behind the swatch so a transparent/
                    // semi-transparent color is actually visible, not just
                    // washed out.
                    Item {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 3
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 2

                        Grid {
                            anchors.fill: parent
                            columns: 6
                            rows: 3
                            Repeater {
                                model: 18
                                Rectangle {
                                    width: parent.width / 6
                                    height: parent.height / 3
                                    color: (Math.floor(index / 6) + (index % 6)) % 2 === 0 ? "#cccccc" : "#ffffff"
                                }
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            color: kcm.outlineColor
                            border.width: 1
                            border.color: Kirigami.Theme.textColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                colorPopup.targetProperty = "outlineColor";
                                colorPopup.open();
                            }
                        }
                    }

                    QQC2.Button {
                        text: "Customize…"
                        onClicked: {
                            colorPopup.targetProperty = "outlineColor";
                            colorPopup.open();
                        }
                    }
                }

                // Indicator color — only shown when not linked to the
                // outline's. Kirigami.FormLayout collapses hidden rows
                // (no blank gap left behind), same pattern as every other
                // conditional row already in this KCM.
                QQC2.ComboBox {
                    Kirigami.FormData.label: "Indicator source:"
                    Layout.preferredWidth: root.fieldWidth
                    visible: !kcm.linkIndicatorOutlineColor
                    textRole: "text"
                    model: [
                        { value: "manual", text: "Manual" },
                        { value: "theme", text: "Theme accent (live)" },
                        { value: "wallpaper", text: "From wallpaper (live)" },
                    ]
                    currentIndex: model.findIndex(m => m.value === kcm.indicatorColorSource)
                    onActivated: kcm.indicatorColorSource = model[currentIndex].value
                }

                RowLayout {
                    Kirigami.FormData.label: "Indicator color:"
                    visible: !kcm.linkIndicatorOutlineColor
                    spacing: Kirigami.Units.largeSpacing

                    Item {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 3
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 2

                        Grid {
                            anchors.fill: parent
                            columns: 6
                            rows: 3
                            Repeater {
                                model: 18
                                Rectangle {
                                    width: parent.width / 6
                                    height: parent.height / 3
                                    color: (Math.floor(index / 6) + (index % 6)) % 2 === 0 ? "#cccccc" : "#ffffff"
                                }
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            color: kcm.indicatorColor
                            border.width: 1
                            border.color: Kirigami.Theme.textColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                colorPopup.targetProperty = "indicatorColor";
                                colorPopup.open();
                            }
                        }
                    }

                    QQC2.Button {
                        text: "Customize…"
                        onClicked: {
                            colorPopup.targetProperty = "indicatorColor";
                            colorPopup.open();
                        }
                    }
                }

                QQC2.TextField {
                    Kirigami.FormData.label: "Wallpaper color file:"
                    placeholderText: "Leave empty to use pywal's default output"
                    text: kcm.wallpaperColorPath
                    onEditingFinished: kcm.wallpaperColorPath = text

                    QQC2.ToolTip.text: "Only used when a color's source above is \"From wallpaper\". Empty reads pywal's own output (~/.cache/wal/colors.json) with no setup needed. A path here is instead read as a single plain hex color (#RRGGBB or #RRGGBBAA) — point a matugen template (or anything else) at it."
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
        // clicked. Read/written only inside the plain JS functions below,
        // never as part of a declarative binding, so the kcm[...] dynamic
        // lookup here is safe (see the note above the Outline ComboBox).
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
