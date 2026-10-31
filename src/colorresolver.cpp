// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "colorresolver.h"
#include "sirkelconfig.h"

#include <KColorScheme>
#include <KSharedConfig>

#include <QDateTime>
#include <QFileInfo>
#include <QStandardPaths>

namespace
{
KSharedConfigPtr globals()
{
    return KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
}

// The accent as last read (invalid until first needed, and again after
// kdeglobals changed): building a KColorScheme reads a whole color group.
QColor &cachedAccent()
{
    static QColor accent;
    return accent;
}

QColor accentColor()
{
    // Plasma applies the accent color by writing it into kdeglobals' color
    // groups, so the selection background is the accent.
    QColor &accent = cachedAccent();
    if (!accent.isValid()) {
        accent = KColorScheme(QPalette::Active, KColorScheme::Selection, globals()).background(KColorScheme::NormalBackground).color();
    }
    return accent;
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
    // Only when it changed since the last time: the effect asks on every
    // trigger press, on the compositor's thread.
    static QDateTime lastModified;
    const QDateTime modified =
        QFileInfo(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/kdeglobals")).lastModified();
    if (modified != lastModified) {
        lastModified = modified;
        globals()->reparseConfiguration();
        cachedAccent() = QColor();
    }
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
    if (source == QStringLiteral("manual")) {
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
