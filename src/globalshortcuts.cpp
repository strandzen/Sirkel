// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "globalshortcuts.h"
#include "shortcutactions.h"
#include "sirkel_debug.h"

#include <KConfigGroup>
#include <KGlobalAccel>
#include <KSharedConfig>

#include <QAction>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QFileInfo>
#include <QStandardPaths>

namespace
{
const QString kShortcutsFileName = QStringLiteral("kglobalshortcutsrc");

// The copy ShortcutsFile keeps open, if any: KSharedConfig hands out the
// same object while one is alive (otherwise this parses the file anew).
KConfigGroup storedGroup(const QString &component)
{
    return KSharedConfig::openConfig(kShortcutsFileName, KConfig::NoGlobals)->group(component);
}

QDateTime shortcutsFileModified()
{
    return QFileInfo(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1Char('/') + kShortcutsFileName).lastModified();
}

// Before 0.5.2, Sirkel's actions were registered under KWin's own group.
constexpr const char *kOldComponent = "kwin";
}

ShortcutsFile::ShortcutsFile()
    : m_config(KSharedConfig::openConfig(kShortcutsFileName, KConfig::NoGlobals))
    , m_modified(shortcutsFileModified())
{
}

bool ShortcutsFile::reloadIfChanged()
{
    const QDateTime modified = shortcutsFileModified();
    if (modified == m_modified) {
        return false;
    }
    m_modified = modified;
    m_config->reparseConfiguration();
    return true;
}

void registerGlobalAction(QAction *action, const QList<QKeySequence> &defaults)
{
    const QString id = action->objectName();
    action->setProperty("componentName", QString::fromLatin1(kShortcutComponent));
    action->setProperty("componentDisplayName", QString::fromLatin1(kShortcutComponentName));
    KGlobalAccel::self()->setDefaultShortcut(action, defaults);

    // Each entry is "keys,default keys,name".
    const QStringList current = storedGroup(QString::fromLatin1(kShortcutComponent)).readEntry(id, QStringList());
    const QStringList old = storedGroup(QString::fromLatin1(kOldComponent)).readEntry(id, QStringList());
    if (current.isEmpty() && !old.isEmpty()) {
        const QList<QKeySequence> keys = parseStoredShortcuts(old.first());
        qCDebug(SIRKEL) << "moving shortcut" << id << "to Sirkel's own group:" << keys;
        KGlobalAccel::self()->setShortcut(action, keys, KGlobalAccel::NoAutoloading);
    } else {
        // Autoloading: a stored binding wins over the defaults.
        KGlobalAccel::self()->setShortcut(action, defaults);
    }
    if (!old.isEmpty()) {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kglobalaccel"),
                                                              QStringLiteral("/kglobalaccel"),
                                                              QStringLiteral("org.kde.KGlobalAccel"),
                                                              QStringLiteral("unregister"));
        message << QString::fromLatin1(kOldComponent) << id;
        QDBusConnection::sessionBus().asyncCall(message);
    }
}

QList<QKeySequence> storedShortcuts(const QString &actionId, const QList<QKeySequence> &defaults)
{
    const QStringList entry = storedGroup(QString::fromLatin1(kShortcutComponent)).readEntry(actionId, QStringList());
    return entry.isEmpty() ? defaults : parseStoredShortcuts(entry.first());
}
