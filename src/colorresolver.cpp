#include "colorresolver.h"
#include "sirkelconfig.h"

#include <KColorScheme>
#include <KSharedConfig>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
// pywal's default output (colors.color1 of ~/.cache/wal/colors.json) when
// WallpaperColorPath is empty, otherwise a file holding one plain hex color
// (covers matugen templates and similar). Invalid QColor if unreadable.
QColor readWallpaperColor()
{
    const QString configuredPath = SirkelConfig::wallpaperColorPath();
    if (configuredPath.isEmpty()) {
        QFile file(QDir::homePath() + QStringLiteral("/.cache/wal/colors.json"));
        if (!file.open(QIODevice::ReadOnly)) {
            return QColor();
        }
        const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        return QColor(root.value(QStringLiteral("colors")).toObject().value(QStringLiteral("color1")).toString());
    }
    QFile file(configuredPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QColor();
    }
    return QColor(QString::fromUtf8(file.readAll()).trimmed());
}

KSharedConfigPtr globals()
{
    return KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
}

QColor accentColor()
{
    // Plasma applies the accent color by writing it into kdeglobals' color
    // groups, so the selection background is the accent.
    return KColorScheme(QPalette::Active, KColorScheme::Selection, globals()).background(KColorScheme::NormalBackground).color();
}

// The accent's hue at the default ring's saturation and brightness
// (#2e3440: about 28% and 25%), so a theme-colored ring stays a dark
// backdrop for the pointer.
QColor ringColorFrom(const QColor &accent)
{
    return QColor::fromHsvF(accent.hsvHueF(), accent.hsvSaturationF() * 0.4, 0.25);
}

}

void reloadColorSources()
{
    globals()->reparseConfiguration();
}

QColor resolveColor(ColorTarget target)
{
    QString source;
    switch (target) {
    case ColorTarget::Outline:
        source = SirkelConfig::outlineColorSource();
        break;
    case ColorTarget::Pointer:
        source = SirkelConfig::indicatorPointerColorSource();
        break;
    case ColorTarget::Ring:
        source = SirkelConfig::indicatorRingColorSource();
        break;
    }
    if (source == QStringLiteral("wallpaper")) {
        const QColor wallpaper = readWallpaperColor();
        if (wallpaper.isValid()) {
            return wallpaper;
        }
    } else if (source == QStringLiteral("manual")) {
        switch (target) {
        case ColorTarget::Outline:
            return SirkelConfig::outlineColor();
        case ColorTarget::Pointer:
            return SirkelConfig::indicatorPointerColor();
        case ColorTarget::Ring:
            return SirkelConfig::indicatorRingColor();
        }
    }
    const QColor accent = accentColor();
    return target == ColorTarget::Ring ? ringColorFrom(accent) : accent;
}
