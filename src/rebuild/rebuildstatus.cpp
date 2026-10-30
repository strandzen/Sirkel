// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rebuildstatus.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginMetaData>

#include <QFileInfo>
#include <QJsonObject>
#include <QPluginLoader>
#include <QStandardPaths>

namespace
{
const QLatin1String kEffectIid("org.kde.kwin.EffectPluginFactory");
}

namespace Rebuild
{

QString versionFromIid(const QString &iid)
{
    return iid.startsWith(kEffectIid) ? iid.mid(kEffectIid.size()) : QString();
}

QString versionFromLibraryLink(const QString &link)
{
    // libkwin.so.6 -> libkwin.so.6.8.1: the version is what follows ".so.".
    const QString file = QFileInfo(QFileInfo(link).canonicalFilePath()).fileName();
    const qsizetype so = file.indexOf(QLatin1String(".so."));
    return so < 0 ? QString() : file.mid(so + 4);
}

QString builtForKWin()
{
    // Found the way KWin finds effects; the metadata is read without
    // loading the plugin.
    const KPluginMetaData plugin = KPluginMetaData::findPluginById(QStringLiteral("kwin/effects/plugins"), QStringLiteral("kwin_effect_sirkel"));
    const QString version = plugin.isValid() ? versionFromIid(QPluginLoader(plugin.fileName()).metaData().value(QLatin1String("IID")).toString()) : QString();
    return version.isEmpty() ? QStringLiteral(SIRKEL_KWIN_VERSION) : version;
}

QString installedKWin()
{
    return versionFromLibraryLink(QStringLiteral(SIRKEL_KWIN_LIBRARY_LINK));
}

bool effectEnabled()
{
    const KConfig kwinrc(QStringLiteral("kwinrc"), KConfig::NoGlobals);
    return KConfigGroup(&kwinrc, QStringLiteral("Plugins")).readEntry("kwin_effect_sirkelEnabled", false);
}

bool needsRebuild(const QString &builtFor, const QString &installed)
{
    return !installed.isEmpty() && installed != builtFor;
}

EffectStatus effectStatus(const QString &builtFor, const QString &installed, bool loaded, bool enabled)
{
    if (needsRebuild(builtFor, installed)) {
        return EffectStatus::Rebuild;
    }
    if (loaded) {
        return EffectStatus::Running;
    }
    return enabled ? EffectStatus::Relogin : EffectStatus::Disabled;
}

QString statusName(EffectStatus status)
{
    switch (status) {
    case EffectStatus::Running:
        return QStringLiteral("running");
    case EffectStatus::Rebuild:
        return QStringLiteral("rebuild");
    case EffectStatus::Relogin:
        return QStringLiteral("relogin");
    case EffectStatus::Disabled:
        break;
    }
    return QStringLiteral("disabled");
}

bool shouldNotify(const QString &builtFor, const QString &installed, bool enabled)
{
    return enabled && needsRebuild(builtFor, installed);
}

QString packageDir()
{
    return QStringLiteral(SIRKEL_REBUILD_DATADIR);
}

bool canRebuild()
{
#if SIRKEL_HAS_REBUILD_HELPER
    return QFileInfo::exists(packageDir() + QStringLiteral("/PKGBUILD"));
#else
    return false;
#endif
}

bool hasBuildTools()
{
    for (const char *tool : {"fakeroot", "strip", "make", "c++"}) {
        if (QStandardPaths::findExecutable(QString::fromLatin1(tool)).isEmpty()) {
            return false;
        }
    }
    return true;
}

RebuildPaths defaultPaths()
{
    const QString cache = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/sirkel");
    return {
        .packageDir = packageDir(),
        .buildDir = cache + QStringLiteral("/rebuild"),
        .lockFile = cache + QStringLiteral("/rebuild.lock"),
        .logFile = cache + QStringLiteral("/rebuild.log"),
        .scriptFile = cache + QStringLiteral("/rebuild.sh"),
    };
}

QString shellQuote(const QString &value)
{
    QString quoted = value;
    quoted.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + quoted + QLatin1Char('\'');
}

QString rebuildScript(const RebuildPaths &paths)
{
    const QString busy = i18n("A rebuild is already running in another window.");
    const QString done = i18n("Sirkel is rebuilt. Log out and back in to start it.");
    const QString failed = i18n("The rebuild didn't finish. Everything above is also saved in %1, which you can attach to a bug report at %2.",
                                paths.logFile,
                                QStringLiteral("https://github.com/strandzen/Sirkel/issues"));
    const QString close = i18n("Press Enter to close this window.");

    // makepkg runs under script(1), which keeps it on a real terminal
    // (colors, pacman's questions, sudo's password prompt) while saving its
    // output. SHELL is set because script runs the command with it, and the
    // user's own shell may not be sh-compatible (fish).
    QString script = QStringLiteral(
        "#!/bin/sh\n"
        "# Written and run by sirkel-rebuild.\n"
        "exec 9>>%1\n"
        "if ! flock -n 9; then\n"
        "    printf '%s\\n' %2\n"
        "else\n"
        "    rm -rf %3 && mkdir -p %3 && cp %4/* %3/ && cd %3 &&\n"
        "        SHELL=/bin/sh script --quiet --return --command 'makepkg -si' %5\n"
        "    status=$?\n"
        "    echo\n"
        "    if [ \"$status\" -eq 0 ]; then\n"
        "        printf '%s\\n' %6\n"
        "    else\n"
        "        printf '%s\\n' %7\n"
        "    fi\n"
        "fi\n"
        "printf '%s' %8\n"
        "read -r _\n");
    return script.arg(shellQuote(paths.lockFile),
                      shellQuote(busy),
                      shellQuote(paths.buildDir),
                      shellQuote(paths.packageDir),
                      shellQuote(paths.logFile),
                      shellQuote(done),
                      shellQuote(failed),
                      shellQuote(close));
}

QStringList closingTerminalCommand(const QString &terminalExec, const QString &scriptFile, const QString &workDir)
{
    if (terminalExec == QLatin1String("konsole")) {
        return {QStringLiteral("konsole"), QStringLiteral("--workdir"), workDir, QStringLiteral("-e"), QStringLiteral("sh"), scriptFile};
    }
    if (terminalExec == QLatin1String("xterm")) {
        return {QStringLiteral("xterm"), QStringLiteral("-e"), QStringLiteral("sh"), scriptFile};
    }
    return {};
}

} // namespace Rebuild
