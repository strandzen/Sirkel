#include "sirkelconfigmodule.h"
#include "sirkelconfig.h"

#include <KConfigGroup>
#include <KPluginFactory>

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>

namespace
{
// See readWallpaperColor() / resolveColor() in sirkeleffect.cpp — identical
// logic, duplicated here (rather than shared) because this is a separate
// process (systemsettings/kcmshell6, not kwin_wayland) with its own copy of
// SirkelConfig::self().
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

QColor resolveColor(bool forOutline)
{
    const QString source = forOutline ? SirkelConfig::outlineColorSource() : SirkelConfig::indicatorColorSource();
    if (source == QStringLiteral("wallpaper")) {
        const QColor wallpaper = readWallpaperColor();
        if (wallpaper.isValid()) {
            return wallpaper;
        }
    } else if (source == QStringLiteral("manual")) {
        return forOutline ? SirkelConfig::outlineColor() : SirkelConfig::indicatorColor();
    }
    return QGuiApplication::palette().color(QPalette::Highlight);
}
}

SirkelConfigModule::SirkelConfigModule(QObject *parent, const KPluginMetaData &metaData)
    : KQuickConfigModule(parent, metaData)
{
    setButtons(KAbstractConfigModule::Default);
}

int SirkelConfigModule::horizontalDeadzone() const
{
    return SirkelConfig::horizontalDeadzone();
}

void SirkelConfigModule::setHorizontalDeadzone(int value)
{
    SirkelConfig::setHorizontalDeadzone(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::verticalDeadzone() const
{
    return SirkelConfig::verticalDeadzone();
}

void SirkelConfigModule::setVerticalDeadzone(int value)
{
    SirkelConfig::setVerticalDeadzone(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::paddingHorizontal() const
{
    return SirkelConfig::paddingHorizontal();
}

void SirkelConfigModule::setPaddingHorizontal(int value)
{
    SirkelConfig::setPaddingHorizontal(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::paddingVertical() const
{
    return SirkelConfig::paddingVertical();
}

void SirkelConfigModule::setPaddingVertical(int value)
{
    SirkelConfig::setPaddingVertical(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::indicatorSize() const
{
    return SirkelConfig::indicatorSize();
}

void SirkelConfigModule::setIndicatorSize(int value)
{
    SirkelConfig::setIndicatorSize(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::indicatorCornerRadius() const
{
    return SirkelConfig::indicatorCornerRadius();
}

void SirkelConfigModule::setIndicatorCornerRadius(int value)
{
    SirkelConfig::setIndicatorCornerRadius(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::indicatorRingWidth() const
{
    return SirkelConfig::indicatorRingWidth();
}

void SirkelConfigModule::setIndicatorRingWidth(int value)
{
    SirkelConfig::setIndicatorRingWidth(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::indicatorPointerLength() const
{
    return SirkelConfig::indicatorPointerLength();
}

void SirkelConfigModule::setIndicatorPointerLength(int value)
{
    SirkelConfig::setIndicatorPointerLength(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::showIndicatorText() const
{
    return SirkelConfig::showIndicatorText();
}

void SirkelConfigModule::setShowIndicatorText(bool value)
{
    SirkelConfig::setShowIndicatorText(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::outlineBorderWidth() const
{
    return SirkelConfig::outlineBorderWidth();
}

void SirkelConfigModule::setOutlineBorderWidth(int value)
{
    SirkelConfig::setOutlineBorderWidth(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::outlineCornerRadius() const
{
    return SirkelConfig::outlineCornerRadius();
}

void SirkelConfigModule::setOutlineCornerRadius(int value)
{
    SirkelConfig::setOutlineCornerRadius(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

QColor SirkelConfigModule::outlineColor() const
{
    return resolveColor(true);
}

void SirkelConfigModule::setOutlineColor(const QColor &value)
{
    // Picking any concrete color (a theme swatch or the custom picker) is a
    // deliberate choice — freeze it, same as picking "Manual" explicitly,
    // rather than keep silently tracking "theme"/"wallpaper" afterward.
    SirkelConfig::setOutlineColor(value);
    SirkelConfig::setOutlineColorSource(QStringLiteral("manual"));
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

QColor SirkelConfigModule::indicatorColor() const
{
    return SirkelConfig::linkIndicatorOutlineColor() ? resolveColor(true) : resolveColor(false);
}

void SirkelConfigModule::setIndicatorColor(const QColor &value)
{
    SirkelConfig::setIndicatorColor(value);
    SirkelConfig::setIndicatorColorSource(QStringLiteral("manual"));
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

QString SirkelConfigModule::outlineColorSource() const
{
    return SirkelConfig::outlineColorSource();
}

void SirkelConfigModule::setOutlineColorSource(const QString &value)
{
    SirkelConfig::setOutlineColorSource(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

QString SirkelConfigModule::indicatorColorSource() const
{
    return SirkelConfig::indicatorColorSource();
}

void SirkelConfigModule::setIndicatorColorSource(const QString &value)
{
    SirkelConfig::setIndicatorColorSource(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::linkIndicatorOutlineColor() const
{
    return SirkelConfig::linkIndicatorOutlineColor();
}

void SirkelConfigModule::setLinkIndicatorOutlineColor(bool value)
{
    SirkelConfig::setLinkIndicatorOutlineColor(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

QString SirkelConfigModule::wallpaperColorPath() const
{
    return SirkelConfig::wallpaperColorPath();
}

void SirkelConfigModule::setWallpaperColorPath(const QString &value)
{
    SirkelConfig::setWallpaperColorPath(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::outlineFillOpacity() const
{
    return SirkelConfig::outlineFillOpacity();
}

void SirkelConfigModule::setOutlineFillOpacity(int value)
{
    SirkelConfig::setOutlineFillOpacity(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::outlineBlur() const
{
    return SirkelConfig::outlineBlur();
}

void SirkelConfigModule::setOutlineBlur(bool value)
{
    SirkelConfig::setOutlineBlur(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::outlineAnimationDuration() const
{
    return SirkelConfig::outlineAnimationDuration();
}

void SirkelConfigModule::setOutlineAnimationDuration(int value)
{
    SirkelConfig::setOutlineAnimationDuration(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

QString SirkelConfigModule::keybindScheme() const
{
    return SirkelConfig::keybindScheme();
}

void SirkelConfigModule::setKeybindScheme(const QString &value)
{
    SirkelConfig::setKeybindScheme(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

QStringList SirkelConfigModule::keybindSchemes() const
{
    return {QStringLiteral("arrows"), QStringLiteral("wasd"), QStringLiteral("hjkl"), QStringLiteral("none")};
}

bool SirkelConfigModule::showIndicator() const
{
    return SirkelConfig::showIndicator();
}

void SirkelConfigModule::setShowIndicator(bool value)
{
    SirkelConfig::setShowIndicator(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::showOutline() const
{
    return SirkelConfig::showOutline();
}

void SirkelConfigModule::setShowOutline(bool value)
{
    SirkelConfig::setShowOutline(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::indicatorFollowsMouse() const
{
    return SirkelConfig::indicatorFollowsMouse();
}

void SirkelConfigModule::setIndicatorFollowsMouse(bool value)
{
    SirkelConfig::setIndicatorFollowsMouse(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::snapToCursorScreen() const
{
    return SirkelConfig::snapToCursorScreen();
}

void SirkelConfigModule::setSnapToCursorScreen(bool value)
{
    SirkelConfig::setSnapToCursorScreen(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

QString SirkelConfigModule::triggerMode() const
{
    return SirkelConfig::triggerMode();
}

void SirkelConfigModule::setTriggerMode(const QString &value)
{
    SirkelConfig::setTriggerMode(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

int SirkelConfigModule::triggerDelay() const
{
    return SirkelConfig::triggerDelay();
}

void SirkelConfigModule::setTriggerDelay(int value)
{
    SirkelConfig::setTriggerDelay(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::scrollResizeEnabled() const
{
    return SirkelConfig::scrollResizeEnabled();
}

void SirkelConfigModule::setScrollResizeEnabled(bool value)
{
    SirkelConfig::setScrollResizeEnabled(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

qreal SirkelConfigModule::scrollResizeSmallPercent() const
{
    return SirkelConfig::scrollResizeSmallPercent();
}

void SirkelConfigModule::setScrollResizeSmallPercent(qreal value)
{
    SirkelConfig::setScrollResizeSmallPercent(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

qreal SirkelConfigModule::scrollResizeLargePercent() const
{
    return SirkelConfig::scrollResizeLargePercent();
}

void SirkelConfigModule::setScrollResizeLargePercent(qreal value)
{
    SirkelConfig::setScrollResizeLargePercent(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

void SirkelConfigModule::load()
{
    Q_EMIT settingsChanged();
}

void SirkelConfigModule::save()
{
    SirkelConfig::self()->save();
}

void SirkelConfigModule::defaults()
{
    SirkelConfig::self()->setDefaults();
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

K_PLUGIN_CLASS_WITH_JSON(SirkelConfigModule, "kwin_effect_sirkel_config.json")

#include "sirkelconfigmodule.moc"
