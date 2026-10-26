// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "sirkelconfig.h"

#include <KConfigWatcher>
#include <KQuickManagedConfigModule>

#include <QColor>
#include <QHash>
#include <QKeySequence>
#include <QPointer>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

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
    // KWin only loads an effect built for its exact version. The KWin
    // version Sirkel was built against, the installed one, and whether the
    // effect runs: "checking", "running", "rebuild" (versions differ),
    // "relogin" (enabled, versions match, not loaded yet) or "disabled".
    // Checked asynchronously when the page opens.
    Q_PROPERTY(QString kwinBuildVersion READ kwinBuildVersion CONSTANT)
    Q_PROPERTY(QString kwinInstalledVersion READ kwinInstalledVersion NOTIFY effectStatusChanged)
    Q_PROPERTY(QString effectStatus READ effectStatus NOTIFY effectStatusChanged)
    // Shortcuts (stored by KGlobalAccel under Sirkel's own component, not in
    // kwinrc). The trigger is either F24 (Caps Lock via keyd) or a key
    // combination; the direct actions have no defaults.
    Q_PROPERTY(QString triggerId READ triggerId CONSTANT)
    Q_PROPERTY(bool triggerIsHyperkey READ triggerIsHyperkey NOTIFY shortcutsChanged)
    // "capslock" (F24), "combination" or "none" (cleared, e.g. in System
    // Settings -> Shortcuts): what the status messages are based on.
    Q_PROPERTY(QString triggerKind READ triggerKind NOTIFY shortcutsChanged)
    Q_PROPERTY(QString triggerText READ triggerText NOTIFY shortcutsChanged)
    // [{ id, text }] for every direct action, in display order; text is the
    // short label.
    Q_PROPERTY(QVariantList shortcutActions READ shortcutActions CONSTANT)
    // [{ id, text }] for the keys that act while the trigger is held
    // (Enter, Backspace by default).
    Q_PROPERTY(QVariantList holdKeyActions READ holdKeyActions CONSTANT)
    // Bumped on every shortcut change, so QML bindings calling shortcut()
    // re-read (a reassignment can change another action's shortcut too).
    Q_PROPERTY(int shortcutsRevision READ shortcutsRevision NOTIFY shortcutsChanged)
    // The primary screen's Plasma wallpaper (an image file), for the
    // preview's background. Empty until Plasma has answered, or if it isn't
    // a plain image wallpaper.
    Q_PROPERTY(QUrl wallpaperUrl READ wallpaperUrl NOTIFY wallpaperUrlChanged)

public:
    SirkelConfigModule(QObject *parent, const KPluginMetaData &metaData);

    SirkelConfig *settings() const;
    QColor resolvedOutlineColor() const;
    QColor resolvedPointerColor() const;
    QColor resolvedRingColor() const;
    QStringList keybindSchemes() const;
    bool keydOk() const;
    QString version() const;
    QString kwinBuildVersion() const;
    QString kwinInstalledVersion() const;
    QString effectStatus() const;
    QString triggerId() const;
    bool triggerIsHyperkey() const;
    QString triggerKind() const;
    QString triggerText() const;
    QVariantList shortcutActions() const;
    QVariantList holdKeyActions() const;
    int shortcutsRevision() const;
    QUrl wallpaperUrl() const;

    // Resizes the dialog hosting this page, animated, so the page ends up
    // `targetPageWidth` wide (the page is `pageWidth` wide now). Used to open
    // the preview to the right without squeezing the settings. Does nothing
    // when the host isn't a widget dialog (e.g. System Settings' own window).
    Q_INVOKABLE void resizeHostWindow(qreal pageWidth, qreal targetPageWidth, int duration);

    // Everything the preview needs for placement `name` (placementName()) on
    // a width x height screen, from the same code the effect uses:
    // { rect, ringIndex }. Padding from the settings.
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
    void effectStatusChanged();
    void wallpaperUrlChanged();
    void shortcutsChanged();

private Q_SLOTS:
    // Also called when Plasma reports a wallpaper change.
    void fetchWallpaper();

private:
    void checkKeyd();
    void reloadShortcuts();
    void checkEffect();

    QTimer *m_saveTimer = nullptr;
    QPointer<QPropertyAnimation> m_resizeAnimation;
    bool m_keydOk = true;
    QString m_kwinInstalledVersion;
    QUrl m_wallpaperUrl;
    QString m_effectStatus = QStringLiteral("checking");
    int m_shortcutsRevision = 0;
    // Action id -> shortcut, see reloadShortcuts().
    QHash<QString, QKeySequence> m_shortcuts;
    KConfigWatcher::Ptr m_globalsWatcher;
};
