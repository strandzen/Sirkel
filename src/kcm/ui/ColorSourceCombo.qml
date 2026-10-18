import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// The manual / theme / wallpaper source dropdown. Never touches `kcm`
// itself: the caller passes the current value in and handles sourcePicked.
QQC2.ComboBox {
    id: root

    // A ComboBox's minimum width follows its widest option, which can push
    // the whole Kirigami.FormLayout into label-above-field mode. Zero lets
    // Layout.preferredWidth win and long text elide instead.
    Layout.minimumWidth: 0

    property string sourceValue
    property var sourceOptions: [
        { value: "manual", text: i18n("Manual") },
        { value: "theme", text: i18n("Theme accent (live)") },
        { value: "wallpaper", text: i18n("From wallpaper (live)") },
    ]

    signal sourcePicked(string value)

    textRole: "text"
    model: sourceOptions
    currentIndex: sourceOptions.findIndex(m => m.value === sourceValue)
    onActivated: sourcePicked(sourceOptions[currentIndex].value)
}
