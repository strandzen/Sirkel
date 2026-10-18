#pragma once

#include "sirkelconfig.h"

#include <KQuickManagedConfigModule>

#include <QColor>
#include <QStringList>

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

public:
    SirkelConfigModule(QObject *parent, const KPluginMetaData &metaData);

    SirkelConfig *settings() const;
    QColor resolvedOutlineColor() const;
    QColor resolvedPointerColor() const;
    QColor resolvedRingColor() const;
    QStringList keybindSchemes() const;
    bool keydOk() const;

    // Picking a concrete color freezes it: stores it and switches that
    // target's source to "manual". `target` is "outline", "pointer" or
    // "ring".
    Q_INVOKABLE void pickColor(const QString &target, const QColor &color);
    // Resets just the Colors tab's settings to their kcfg defaults.
    Q_INVOKABLE void resetColors();

    void save() override;

Q_SIGNALS:
    void colorsChanged();
    void keydOkChanged();

private:
    void checkKeyd();

    QTimer *m_saveTimer = nullptr;
    bool m_keydOk = true;
};
