import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// The "manual / theme / wallpaper" source dropdown, reused for the
// Outline/Pointer/Ring color sources in the Colors tab. `sourceValue` is
// the current kcfg string, passed in by the caller; `sourcePicked` fires
// with the newly-chosen value for the caller to write back — this
// component never touches `kcm` directly, so it has no idea which target
// it belongs to, keeping it fully reusable. `sourceOptions` defaults to
// all three choices; Ring's own instance overrides it to drop "wallpaper"
// (see Ring source's comment in main.qml for why).
QQC2.ComboBox {
    id: root

    // Qt Layouts lets a child's own content-driven Layout.minimumWidth
    // override a smaller Layout.preferredWidth set by the caller - a
    // ComboBox's minimumWidth normally follows its own implicitWidth
    // (sized to fit its widest option, e.g. "From wallpaper (live)"),
    // which can silently win over the caller's intended fieldWidth and
    // push a whole FormLayout row (or, confirmed live, the *entire* form -
    // Kirigami.FormLayout shares one column-width budget across every row)
    // wider than available, forcing everything to wrap to label-above-
    // field. Zeroing it here hands control back to Layout.preferredWidth,
    // letting long option text elide instead.
    Layout.minimumWidth: 0

    property string sourceValue
    property var sourceOptions: [
        { value: "manual", text: "Manual" },
        { value: "theme", text: "Theme accent (live)" },
        { value: "wallpaper", text: "From wallpaper (live)" },
    ]

    signal sourcePicked(string value)

    textRole: "text"
    model: sourceOptions
    currentIndex: sourceOptions.findIndex(m => m.value === sourceValue)
    onActivated: sourcePicked(sourceOptions[currentIndex].value)
}
