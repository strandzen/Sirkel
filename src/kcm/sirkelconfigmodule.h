#pragma once

#include "sirkelconfig.h"

#include <KQuickManagedConfigModule>

#include <QColor>
#include <QPointer>
#include <QKeySequence>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>

class QPropertyAnimation;
class QTimer;

// System Settings page for Sirkel (Desktop Effects -> Sirkel's configure
// button, wired via metadata.json's X-KDE-ConfigModule).
//
// Plain settings are exposed directly as `kcm.settings.<name>` (the
// generated SirkelConfig properties). Changes apply instantly: a short
// debounce after any change saves and asks KWin to reconfigure the effect.
class SirkelConfigModule : public KQuickManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(SirkelConfig *settings READ settings CONSTANT)
    // The colors as the effect will actually draw them (sources and the
    // link toggle applied), for the swatches.
    Q_PROPERTY(QColor resolvedOutlineColor READ resolvedOutlineColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor resolvedPointerColor READ resolvedPointerColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor resolvedRingColor READ resolvedRingColor NOTIFY colorsChanged)
    Q_PROPERTY(QStringList keybindSchemes READ keybindSchemes CONSTANT)
    // False only when keyd is definitely not set up (service inactive, or
    // readable config with no capslock -> f24 mapping). Checked
    // asynchronously when the page opens.
    Q_PROPERTY(bool keydOk READ keydOk NOTIFY keydOkChanged)
    // Plugin version, for the About page.
    Q_PROPERTY(QString version READ version CONSTANT)
    // Shortcuts (stored by KGlobalAccel under the "kwin" component, not in
    // kwinrc). The trigger is either F24 (Caps Lock via keyd) or a key
    // combination; the direct actions have no defaults.
    Q_PROPERTY(QString triggerId READ triggerId CONSTANT)
    Q_PROPERTY(bool triggerIsHyperkey READ triggerIsHyperkey NOTIFY shortcutsChanged)
    Q_PROPERTY(QString triggerText READ triggerText NOTIFY shortcutsChanged)
    // [{ id, text }] for every direct action, in display order; text is the
    // short label.
    Q_PROPERTY(QVariantList shortcutActions READ shortcutActions CONSTANT)
    // Bumped on every shortcut change, so QML bindings calling shortcut()
    // re-read (a reassignment can change another action's shortcut too).
    Q_PROPERTY(int shortcutsRevision READ shortcutsRevision NOTIFY shortcutsChanged)
    // The primary screen's Plasma wallpaper (an image file), for the
    // preview's background. Empty if it isn't a plain image wallpaper.
    Q_PROPERTY(QUrl wallpaperUrl READ wallpaperUrl CONSTANT)

public:
    SirkelConfigModule(QObject *parent, const KPluginMetaData &metaData);

    SirkelConfig *settings() const;
    QColor resolvedOutlineColor() const;
    QColor resolvedPointerColor() const;
    QColor resolvedRingColor() const;
    QStringList keybindSchemes() const;
    bool keydOk() const;
    QString version() const;
    QString triggerId() const;
    bool triggerIsHyperkey() const;
    QString triggerText() const;
    QVariantList shortcutActions() const;
    int shortcutsRevision() const;
    QUrl wallpaperUrl() const;

    // Resizes the dialog hosting this page, animated, so the page ends up
    // `targetPageWidth` wide (the page is `pageWidth` wide now). Used to open
    // the preview to the right without squeezing the settings. Does nothing
    // when the host isn't a widget dialog (e.g. System Settings' own window).
    Q_INVOKABLE void resizeHostWindow(qreal pageWidth, qreal targetPageWidth, int duration);

    // Everything the preview needs for placement `name` (placementName()) on
    // a width x height screen, from the same code the effect uses:
    // { rect, ringIndex, label, edgeH, edgeV }. Padding from the settings.
    Q_INVOKABLE QVariantMap previewPlacement(const QString &name, qreal width, qreal height) const;

    Q_INVOKABLE QKeySequence shortcut(const QString &actionId) const;
    // Applies immediately (KGlobalAccel isn't part of the Apply/Defaults
    // cycle). An empty sequence clears the shortcut.
    Q_INVOKABLE void setShortcut(const QString &actionId, const QKeySequence &sequence);
    // Back to Caps Lock: the trigger becomes F24 again.
    Q_INVOKABLE void useHyperkeyTrigger();

    // Picking a concrete color freezes it: stores it and switches that
    // target's source to "manual". `target` is "outline", "pointer" or
    // "ring".
    Q_INVOKABLE void pickColor(const QString &target, const QColor &color);
    // Resets just the Colors tab's settings to their kcfg defaults.
    Q_INVOKABLE void resetColors();
    // Opens System Settings -> Shortcuts, where the trigger can be rebound.
    Q_INVOKABLE void openShortcutsSettings();

    void save() override;

Q_SIGNALS:
    void colorsChanged();
    void keydOkChanged();
    void shortcutsChanged();

private:
    void checkKeyd();

    QTimer *m_saveTimer = nullptr;
    QPointer<QPropertyAnimation> m_resizeAnimation;
    bool m_keydOk = true;
    int m_shortcutsRevision = 0;
};
