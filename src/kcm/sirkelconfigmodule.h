#pragma once

#include <KQuickConfigModule>

#include <QColor>
#include <QStringList>

// System Settings page for Sirkel, under Desktop Effects' configure
// button for the effect (wired via metadata.json's X-KDE-ConfigModule).
// Every property here is a thin, uncached read/write pass-through to
// SirkelConfig::self() — "instant apply" (writes + saves immediately on
// change), per the documented KQuickConfigModule pattern of exposing
// settings as plain properties on the module itself, accessed from QML via
// the framework-provided "kcm" object.
class SirkelConfigModule : public KQuickConfigModule
{
    Q_OBJECT

    Q_PROPERTY(int horizontalDeadzone READ horizontalDeadzone WRITE setHorizontalDeadzone NOTIFY settingsChanged)
    Q_PROPERTY(int verticalDeadzone READ verticalDeadzone WRITE setVerticalDeadzone NOTIFY settingsChanged)
    Q_PROPERTY(int paddingHorizontal READ paddingHorizontal WRITE setPaddingHorizontal NOTIFY settingsChanged)
    Q_PROPERTY(int paddingVertical READ paddingVertical WRITE setPaddingVertical NOTIFY settingsChanged)
    Q_PROPERTY(int indicatorSize READ indicatorSize WRITE setIndicatorSize NOTIFY settingsChanged)
    Q_PROPERTY(int indicatorCornerRadius READ indicatorCornerRadius WRITE setIndicatorCornerRadius NOTIFY settingsChanged)
    Q_PROPERTY(int indicatorRingWidth READ indicatorRingWidth WRITE setIndicatorRingWidth NOTIFY settingsChanged)
    Q_PROPERTY(int indicatorPointerLength READ indicatorPointerLength WRITE setIndicatorPointerLength NOTIFY settingsChanged)
    Q_PROPERTY(bool showIndicatorText READ showIndicatorText WRITE setShowIndicatorText NOTIFY settingsChanged)
    Q_PROPERTY(int indicatorRingFillOpacity READ indicatorRingFillOpacity WRITE setIndicatorRingFillOpacity NOTIFY settingsChanged)
    Q_PROPERTY(int outlineBorderWidth READ outlineBorderWidth WRITE setOutlineBorderWidth NOTIFY settingsChanged)
    Q_PROPERTY(int outlineCornerRadius READ outlineCornerRadius WRITE setOutlineCornerRadius NOTIFY settingsChanged)
    Q_PROPERTY(QColor outlineColor READ outlineColor WRITE setOutlineColor NOTIFY settingsChanged)
    Q_PROPERTY(QColor indicatorPointerColor READ indicatorPointerColor WRITE setIndicatorPointerColor NOTIFY settingsChanged)
    Q_PROPERTY(QColor indicatorRingColor READ indicatorRingColor WRITE setIndicatorRingColor NOTIFY settingsChanged)
    Q_PROPERTY(QString outlineColorSource READ outlineColorSource WRITE setOutlineColorSource NOTIFY settingsChanged)
    Q_PROPERTY(QString indicatorPointerColorSource READ indicatorPointerColorSource WRITE setIndicatorPointerColorSource NOTIFY settingsChanged)
    Q_PROPERTY(QString indicatorRingColorSource READ indicatorRingColorSource WRITE setIndicatorRingColorSource NOTIFY settingsChanged)
    Q_PROPERTY(bool linkIndicatorColors READ linkIndicatorColors WRITE setLinkIndicatorColors NOTIFY settingsChanged)
    Q_PROPERTY(QString wallpaperColorPath READ wallpaperColorPath WRITE setWallpaperColorPath NOTIFY settingsChanged)
    Q_PROPERTY(int outlineFillOpacity READ outlineFillOpacity WRITE setOutlineFillOpacity NOTIFY settingsChanged)
    Q_PROPERTY(bool outlineBlur READ outlineBlur WRITE setOutlineBlur NOTIFY settingsChanged)
    Q_PROPERTY(int outlineAnimationDuration READ outlineAnimationDuration WRITE setOutlineAnimationDuration NOTIFY settingsChanged)
    Q_PROPERTY(QString keybindScheme READ keybindScheme WRITE setKeybindScheme NOTIFY settingsChanged)
    Q_PROPERTY(QStringList keybindSchemes READ keybindSchemes CONSTANT)
    Q_PROPERTY(bool showIndicator READ showIndicator WRITE setShowIndicator NOTIFY settingsChanged)
    Q_PROPERTY(bool showOutline READ showOutline WRITE setShowOutline NOTIFY settingsChanged)
    Q_PROPERTY(bool indicatorFollowsMouse READ indicatorFollowsMouse WRITE setIndicatorFollowsMouse NOTIFY settingsChanged)
    Q_PROPERTY(bool snapToCursorScreen READ snapToCursorScreen WRITE setSnapToCursorScreen NOTIFY settingsChanged)
    Q_PROPERTY(QString triggerMode READ triggerMode WRITE setTriggerMode NOTIFY settingsChanged)
    Q_PROPERTY(int triggerDelay READ triggerDelay WRITE setTriggerDelay NOTIFY settingsChanged)
    Q_PROPERTY(bool scrollResizeEnabled READ scrollResizeEnabled WRITE setScrollResizeEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString easing READ easing WRITE setEasing NOTIFY settingsChanged)
    Q_PROPERTY(bool outlineConfirmFlash READ outlineConfirmFlash WRITE setOutlineConfirmFlash NOTIFY settingsChanged)
    Q_PROPERTY(bool outlineGradientFill READ outlineGradientFill WRITE setOutlineGradientFill NOTIFY settingsChanged)

public:
    explicit SirkelConfigModule(QObject *parent, const KPluginMetaData &metaData);

    int horizontalDeadzone() const;
    void setHorizontalDeadzone(int value);

    int verticalDeadzone() const;
    void setVerticalDeadzone(int value);

    int paddingHorizontal() const;
    void setPaddingHorizontal(int value);

    int paddingVertical() const;
    void setPaddingVertical(int value);

    int indicatorSize() const;
    void setIndicatorSize(int value);

    int indicatorCornerRadius() const;
    void setIndicatorCornerRadius(int value);

    int indicatorRingWidth() const;
    void setIndicatorRingWidth(int value);

    int indicatorPointerLength() const;
    void setIndicatorPointerLength(int value);

    bool showIndicatorText() const;
    void setShowIndicatorText(bool value);

    int indicatorRingFillOpacity() const;
    void setIndicatorRingFillOpacity(int value);

    int outlineBorderWidth() const;
    void setOutlineBorderWidth(int value);

    int outlineCornerRadius() const;
    void setOutlineCornerRadius(int value);

    QColor outlineColor() const;
    void setOutlineColor(const QColor &value);

    QColor indicatorPointerColor() const;
    void setIndicatorPointerColor(const QColor &value);

    QColor indicatorRingColor() const;
    void setIndicatorRingColor(const QColor &value);

    QString outlineColorSource() const;
    void setOutlineColorSource(const QString &value);

    QString indicatorPointerColorSource() const;
    void setIndicatorPointerColorSource(const QString &value);

    QString indicatorRingColorSource() const;
    void setIndicatorRingColorSource(const QString &value);

    bool linkIndicatorColors() const;
    void setLinkIndicatorColors(bool value);

    QString wallpaperColorPath() const;
    void setWallpaperColorPath(const QString &value);

    int outlineFillOpacity() const;
    void setOutlineFillOpacity(int value);

    bool outlineBlur() const;
    void setOutlineBlur(bool value);

    int outlineAnimationDuration() const;
    void setOutlineAnimationDuration(int value);

    // Stored value, not display label — presets intentionally fixed rather
    // than arbitrary rebinding, see Todo.md's keybinds item.
    QString keybindScheme() const;
    void setKeybindScheme(const QString &value);
    QStringList keybindSchemes() const;

    bool showIndicator() const;
    void setShowIndicator(bool value);

    bool showOutline() const;
    void setShowOutline(bool value);

    bool indicatorFollowsMouse() const;
    void setIndicatorFollowsMouse(bool value);

    bool snapToCursorScreen() const;
    void setSnapToCursorScreen(bool value);

    QString triggerMode() const;
    void setTriggerMode(const QString &value);

    int triggerDelay() const;
    void setTriggerDelay(int value);

    bool scrollResizeEnabled() const;
    void setScrollResizeEnabled(bool value);

    QString easing() const;
    void setEasing(const QString &value);

    bool outlineConfirmFlash() const;
    void setOutlineConfirmFlash(bool value);

    bool outlineGradientFill() const;
    void setOutlineGradientFill(bool value);

    // Resets just the Colors tab's own settings (both colors, both
    // sources, the link toggle, and the wallpaper path) to their kcfg
    // defaults — distinct from the KCM's built-in "Defaults" button, which
    // resets every setting on every tab. Uses each entry's own
    // KConfigSkeletonItem::setDefault() (ItemAccessors=true in
    // sirkelconfig.kcfgc) rather than re-typing the default values here by
    // hand, so this can't drift out of sync with the kcfg file.
    Q_INVOKABLE void resetColors();

    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void settingsChanged();
};
