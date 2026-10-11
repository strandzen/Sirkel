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
    Q_PROPERTY(int outlineBorderWidth READ outlineBorderWidth WRITE setOutlineBorderWidth NOTIFY settingsChanged)
    Q_PROPERTY(int outlineCornerRadius READ outlineCornerRadius WRITE setOutlineCornerRadius NOTIFY settingsChanged)
    Q_PROPERTY(QColor outlineColor READ outlineColor WRITE setOutlineColor NOTIFY settingsChanged)
    Q_PROPERTY(QColor indicatorColor READ indicatorColor WRITE setIndicatorColor NOTIFY settingsChanged)
    Q_PROPERTY(QString outlineColorSource READ outlineColorSource WRITE setOutlineColorSource NOTIFY settingsChanged)
    Q_PROPERTY(QString indicatorColorSource READ indicatorColorSource WRITE setIndicatorColorSource NOTIFY settingsChanged)
    Q_PROPERTY(bool linkIndicatorOutlineColor READ linkIndicatorOutlineColor WRITE setLinkIndicatorOutlineColor NOTIFY settingsChanged)
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
    Q_PROPERTY(qreal scrollResizeSmallPercent READ scrollResizeSmallPercent WRITE setScrollResizeSmallPercent NOTIFY settingsChanged)
    Q_PROPERTY(qreal scrollResizeLargePercent READ scrollResizeLargePercent WRITE setScrollResizeLargePercent NOTIFY settingsChanged)

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

    int outlineBorderWidth() const;
    void setOutlineBorderWidth(int value);

    int outlineCornerRadius() const;
    void setOutlineCornerRadius(int value);

    QColor outlineColor() const;
    void setOutlineColor(const QColor &value);

    QColor indicatorColor() const;
    void setIndicatorColor(const QColor &value);

    QString outlineColorSource() const;
    void setOutlineColorSource(const QString &value);

    QString indicatorColorSource() const;
    void setIndicatorColorSource(const QString &value);

    bool linkIndicatorOutlineColor() const;
    void setLinkIndicatorOutlineColor(bool value);

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

    qreal scrollResizeSmallPercent() const;
    void setScrollResizeSmallPercent(qreal value);

    qreal scrollResizeLargePercent() const;
    void setScrollResizeLargePercent(qreal value);

    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void settingsChanged();
};
