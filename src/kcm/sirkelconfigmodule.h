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
    // version the installed Sirkel was built for, the installed one, and
    // whether the effect runs: "checking", "running", "rebuild" (versions
    // differ), "relogin" (enabled, versions match, not loaded yet) or
    // "disabled". Checked when the settings open and when the window is
    // focused again on a page that shows it (see refreshStatus()).
    Q_PROPERTY(QString kwinBuildVersion READ kwinBuildVersion NOTIFY effectStatusChanged)
    Q_PROPERTY(QString kwinInstalledVersion READ kwinInstalledVersion NOTIFY effectStatusChanged)
    Q_PROPERTY(QString effectStatus READ effectStatus NOTIFY effectStatusChanged)
    // The Rebuild button (sirkel-rebuild, src/rebuild/): only for package
    // installs, which carry their PKGBUILD; base-devel is needed to build.
    // The log is the last rebuild's output, empty if there was none.
    Q_PROPERTY(bool canRebuild READ canRebuild NOTIFY effectStatusChanged)
    Q_PROPERTY(bool hasBuildTools READ hasBuildTools NOTIFY effectStatusChanged)
    Q_PROPERTY(QUrl rebuildLogUrl READ rebuildLogUrl NOTIFY effectStatusChanged)
    // The page to show, from `kcmshell6 ... --args about` (the rebuild
    // notification's Open settings): "about" or empty.
    Q_PROPERTY(QString requestedPage READ requestedPage NOTIFY requestedPageChanged)
    // Shortcuts (stored by KGlobalAccel under Sirkel's own component, not in
    // kwinrc). The trigger is either F24 (Caps Lock via keyd) or a key
    // combination; the direct actions have no defaults.
    Q_PROPERTY(QString triggerId READ triggerId CONSTANT)
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
    // Plasma's global animation speed (0 = instant), which the real overlay
    // follows too, so the preview moves at the same pace.
    Q_PROPERTY(qreal animationFactor READ animationFactor NOTIFY animationFactorChanged)

public:
    SirkelConfigModule(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);

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
    bool canRebuild() const;
    bool hasBuildTools() const;
    QUrl rebuildLogUrl() const;
    QString requestedPage() const;
    QString triggerId() const;
    QString triggerKind() const;
    QString triggerText() const;
    QVariantList shortcutActions() const;
    QVariantList holdKeyActions() const;
    int shortcutsRevision() const;
    QUrl wallpaperUrl() const;
    qreal animationFactor() const;

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
    // Checks again whether Sirkel runs and keyd is set up, e.g. when the
    // window is focused again after a rebuild in a terminal.
    Q_INVOKABLE void refreshStatus();
    // Opens a terminal that rebuilds and reinstalls the package.
    Q_INVOKABLE void rebuild();

    void save() override;

Q_SIGNALS:
    void colorsChanged();
    void keydOkChanged();
    void effectStatusChanged();
    void wallpaperUrlChanged();
    void shortcutsChanged();
    void requestedPageChanged();
    void animationFactorChanged();

private Q_SLOTS:
    // Also called when Plasma reports a wallpaper change.
    void fetchWallpaper();

private:
    void checkKeyd();
    void reloadShortcuts();
    void checkEffect();
    void setRequestedPage(const QVariantList &args);

    QTimer *m_saveTimer = nullptr;
    QPointer<QPropertyAnimation> m_resizeAnimation;
    bool m_keydOk = true;
    QString m_kwinBuildVersion;
    QString m_kwinInstalledVersion;
    QUrl m_wallpaperUrl;
    QString m_effectStatus = QStringLiteral("checking");
    bool m_canRebuild = false;
    bool m_hasBuildTools = false;
    QUrl m_rebuildLogUrl;
    QString m_requestedPage;
    int m_shortcutsRevision = 0;
    // Action id -> shortcut, see reloadShortcuts().
    QHash<QString, QKeySequence> m_shortcuts;
    KConfigWatcher::Ptr m_globalsWatcher;
};
