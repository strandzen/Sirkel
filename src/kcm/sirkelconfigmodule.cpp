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
#include <QProcess>
#include <QRegularExpression>

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

enum class ColorTarget { Outline, Pointer, Ring };

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
    return QGuiApplication::palette().color(QPalette::Highlight);
}

// Best-effort only: /etc/keyd/ may not be readable by this process, and
// the mapping could legitimately live under a different key name than
// "capslock" (e.g. remapped again upstream of keyd) - a false negative
// here just means a warning the user can ignore, not a hard failure.
bool checkKeydOk()
{
    const bool serviceActive = QProcess::execute(QStringLiteral("systemctl"), {QStringLiteral("is-active"), QStringLiteral("--quiet"), QStringLiteral("keyd")}) == 0;
    if (!serviceActive) {
        return false;
    }

    static const QRegularExpression mapping(QStringLiteral("^\\s*capslock\\s*=\\s*f24\\s*$"), QRegularExpression::CaseInsensitiveOption);
    QDir keydDir(QStringLiteral("/etc/keyd"));
    const QStringList confFiles = keydDir.entryList({QStringLiteral("*.conf")}, QDir::Files);
    for (const QString &fileName : confFiles) {
        QFile file(keydDir.filePath(fileName));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        while (!file.atEnd()) {
            const QString line = QString::fromUtf8(file.readLine()).trimmed();
            if (line.startsWith(QLatin1Char('#'))) {
                continue;
            }
            if (mapping.match(line).hasMatch()) {
                return true;
            }
        }
    }
    return false;
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

int SirkelConfigModule::indicatorRingFillOpacity() const
{
    return SirkelConfig::indicatorRingFillOpacity();
}

void SirkelConfigModule::setIndicatorRingFillOpacity(int value)
{
    SirkelConfig::setIndicatorRingFillOpacity(value);
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
    return resolveColor(ColorTarget::Outline);
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

QColor SirkelConfigModule::indicatorPointerColor() const
{
    return SirkelConfig::linkIndicatorColors() ? resolveColor(ColorTarget::Outline) : resolveColor(ColorTarget::Pointer);
}

void SirkelConfigModule::setIndicatorPointerColor(const QColor &value)
{
    SirkelConfig::setIndicatorPointerColor(value);
    SirkelConfig::setIndicatorPointerColorSource(QStringLiteral("manual"));
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

QColor SirkelConfigModule::indicatorRingColor() const
{
    return SirkelConfig::linkIndicatorColors() ? resolveColor(ColorTarget::Outline) : resolveColor(ColorTarget::Ring);
}

void SirkelConfigModule::setIndicatorRingColor(const QColor &value)
{
    SirkelConfig::setIndicatorRingColor(value);
    SirkelConfig::setIndicatorRingColorSource(QStringLiteral("manual"));
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

QString SirkelConfigModule::indicatorPointerColorSource() const
{
    return SirkelConfig::indicatorPointerColorSource();
}

void SirkelConfigModule::setIndicatorPointerColorSource(const QString &value)
{
    SirkelConfig::setIndicatorPointerColorSource(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

QString SirkelConfigModule::indicatorRingColorSource() const
{
    return SirkelConfig::indicatorRingColorSource();
}

void SirkelConfigModule::setIndicatorRingColorSource(const QString &value)
{
    SirkelConfig::setIndicatorRingColorSource(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::linkIndicatorColors() const
{
    return SirkelConfig::linkIndicatorColors();
}

void SirkelConfigModule::setLinkIndicatorColors(bool value)
{
    SirkelConfig::setLinkIndicatorColors(value);
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

QString SirkelConfigModule::easing() const
{
    return SirkelConfig::easing();
}

void SirkelConfigModule::setEasing(const QString &value)
{
    SirkelConfig::setEasing(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::outlineConfirmFlash() const
{
    return SirkelConfig::outlineConfirmFlash();
}

void SirkelConfigModule::setOutlineConfirmFlash(bool value)
{
    SirkelConfig::setOutlineConfirmFlash(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::outlineGradientFill() const
{
    return SirkelConfig::outlineGradientFill();
}

void SirkelConfigModule::setOutlineGradientFill(bool value)
{
    SirkelConfig::setOutlineGradientFill(value);
    SirkelConfig::self()->save();
    Q_EMIT settingsChanged();
}

bool SirkelConfigModule::keydOk() const
{
    return checkKeydOk();
}

void SirkelConfigModule::resetColors()
{
    SirkelConfig::self()->outlineColorItem()->setDefault();
    SirkelConfig::self()->outlineColorSourceItem()->setDefault();
    SirkelConfig::self()->indicatorPointerColorItem()->setDefault();
    SirkelConfig::self()->indicatorPointerColorSourceItem()->setDefault();
    SirkelConfig::self()->indicatorRingColorItem()->setDefault();
    SirkelConfig::self()->indicatorRingColorSourceItem()->setDefault();
    SirkelConfig::self()->linkIndicatorColorsItem()->setDefault();
    SirkelConfig::self()->wallpaperColorPathItem()->setDefault();
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
