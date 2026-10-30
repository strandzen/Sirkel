// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// sirkel-rebuild: KWin silently refuses to load Sirkel after every KWin
// update, and the effect can't warn about its own failure. This runs on its
// own instead:
//   --check  at login (autostart): a notification when Sirkel needs a
//            rebuild, with Rebuild and Open settings buttons.
//   (none)   the Rebuild button (notification or About page): opens a
//            terminal that builds and installs the package again.

#include "rebuildstatus.h"
#include "sirkel_rebuild_debug.h"

#include <KConfigGroup>
#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KNotification>
#include <KService>
#include <KSharedConfig>
#include <KShell>
#include <KTerminalLauncherJob>

#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QStandardPaths>

#include <algorithm>
#include <memory>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

namespace
{

// Event ids, see sirkel.notifyrc.
const QString kRebuildNeeded = QStringLiteral("rebuildNeeded");
const QString kRebuildProblem = QStringLiteral("rebuildProblem");

void notifyProblem(const QString &text)
{
    auto *notification = new KNotification(kRebuildProblem);
    notification->setComponentName(QStringLiteral("sirkel"));
    notification->setTitle(i18n("Sirkel"));
    notification->setText(text);
    QObject::connect(notification, &KNotification::closed, qApp, &QCoreApplication::quit);
    notification->sendEvent();
}

// A rebuild still running holds the lock (the terminal's script takes it).
bool rebuildRunning(const QString &lockFile)
{
    const int fd = ::open(QFile::encodeName(lockFile).constData(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    const bool held = ::flock(fd, LOCK_EX | LOCK_NB) != 0;
    ::close(fd);
    return held;
}

void openSettings()
{
    QProcess::startDetached(QStringLiteral("kcmshell6"),
                            {QStringLiteral("kwin/effects/configs/kwin_effect_sirkel_config"), QStringLiteral("--args"), QStringLiteral("about")});
}

// The Exec of the terminal KTerminalLauncherJob would open, picked the same
// way: the default terminal (System Settings -> Default Applications), else
// Konsole, else xterm. Empty if there's none.
QString terminalExec()
{
    const KConfigGroup general(KSharedConfig::openConfig(), QStringLiteral("General"));
    const QString storageId = general.readEntry("TerminalService");
    const QString application = general.readEntry("TerminalApplication");
    KService::Ptr service;
    if (!storageId.isEmpty()) {
        service = KService::serviceByStorageId(storageId);
    } else if (!application.isEmpty()) {
        return application;
    }
    if (!service) {
        service = KService::serviceByStorageId(QStringLiteral("org.kde.konsole"));
    }
    if (service) {
        return service->exec();
    }
    for (const QString &fallback : {QStringLiteral("konsole"), QStringLiteral("xterm")}) {
        if (!QStandardPaths::findExecutable(fallback).isEmpty()) {
            return fallback;
        }
    }
    return {};
}

// Writes the script and opens it in the user's terminal; quits when the
// terminal has started (or a problem was reported).
void startRebuild()
{
    const Rebuild::RebuildPaths paths = Rebuild::defaultPaths();
    if (!Rebuild::canRebuild()) {
        notifyProblem(i18n("Sirkel can only rebuild itself when it was installed as a package. See the About page in its settings for how to rebuild it."));
        return;
    }
    if (rebuildRunning(paths.lockFile)) {
        notifyProblem(i18n("A rebuild of Sirkel is already running."));
        return;
    }

    QDir().mkpath(QFileInfo(paths.scriptFile).path());
    QFile script(paths.scriptFile);
    if (!script.open(QIODevice::WriteOnly | QIODevice::Truncate) || script.write(Rebuild::rebuildScript(paths).toUtf8()) < 0) {
        qCWarning(SIRKEL_REBUILD) << "can't write" << paths.scriptFile << script.errorString();
        notifyProblem(i18n("Couldn't prepare the rebuild: %1 can't be written.", paths.scriptFile));
        return;
    }
    script.close();

    const QString workDir = QFileInfo(paths.scriptFile).path();
    KJob *job;
    if (const QStringList command = Rebuild::closingTerminalCommand(terminalExec(), paths.scriptFile, workDir); !command.isEmpty()) {
        auto *own = new KIO::CommandLauncherJob(command.first(), command.mid(1));
        own->setWorkingDirectory(workDir);
        if (command.first() == QLatin1String("konsole")) {
            own->setDesktopName(QStringLiteral("org.kde.konsole"));
        }
        job = own;
    } else {
        auto *launcher = new KTerminalLauncherJob(QStringLiteral("sh ") + KShell::quoteArg(paths.scriptFile));
        launcher->setWorkingDirectory(workDir);
        job = launcher;
    }
    QObject::connect(job, &KJob::result, qApp, [job]() {
        if (job->error()) {
            qCWarning(SIRKEL_REBUILD) << "can't open a terminal:" << job->errorString();
            notifyProblem(i18n("Couldn't open a terminal for the rebuild: %1", job->errorString()));
            return;
        }
        qCDebug(SIRKEL_REBUILD) << "rebuild started in a terminal";
        QCoreApplication::quit();
    });
    job->start();
}

// The login check found that Sirkel needs a rebuild.
void notifyRebuildNeeded(const QString &builtFor, const QString &installed)
{
    // Stays until dismissed: Sirkel doesn't work until it's rebuilt.
    auto *notification = new KNotification(kRebuildNeeded, KNotification::Persistent);
    notification->setComponentName(QStringLiteral("sirkel"));
    notification->setTitle(i18n("Sirkel needs a rebuild"));
    const QString text = i18n("KWin %1 is installed, but Sirkel was built for KWin %2, so KWin doesn't load it.", installed, builtFor);

    // Rebuild quits once the terminal is open; everything else quits when
    // the notification closes.
    auto rebuilding = std::make_shared<bool>(false);
    if (Rebuild::canRebuild() && Rebuild::hasBuildTools()) {
        notification->setText(text);
        KNotificationAction *rebuild = notification->addAction(i18n("Rebuild for KWin %1", installed));
        QObject::connect(rebuild, &KNotificationAction::activated, qApp, [rebuilding]() {
            *rebuilding = true;
            startRebuild();
        });
    } else if (Rebuild::canRebuild()) {
        notification->setText(text + QLatin1Char(' ') + i18n("To rebuild it, first install the build tools: sudo pacman -S --needed base-devel"));
    } else {
        notification->setText(text);
    }
    KNotificationAction *settings = notification->addAction(i18n("Open settings"));
    QObject::connect(settings, &KNotificationAction::activated, qApp, &openSettings);
    // Clicking the notification itself opens the settings too.
    KNotificationAction *open = notification->addDefaultAction(i18n("Open settings"));
    QObject::connect(open, &KNotificationAction::activated, qApp, &openSettings);
    QObject::connect(notification, &KNotification::closed, qApp, [rebuilding]() {
        if (!*rebuilding) {
            QCoreApplication::quit();
        }
    });
    notification->sendEvent();
}

} // namespace

int main(int argc, char **argv)
{
    // The login check is decided from a few file reads before anything is
    // started: almost every login has nothing to report.
    const bool check = std::any_of(argv + 1, argv + argc, [](const char *arg) {
        return qstrcmp(arg, "--check") == 0;
    });
    const QString builtFor = Rebuild::builtForKWin();
    const QString installed = Rebuild::installedKWin();
    if (check) {
        const bool enabled = Rebuild::effectEnabled();
        qCDebug(SIRKEL_REBUILD) << "built for KWin" << builtFor << "installed" << installed << "enabled" << enabled;
        if (!Rebuild::shouldNotify(builtFor, installed, enabled)) {
            return 0;
        }
    }

    QGuiApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("sirkel");
    app.setApplicationName(QStringLiteral("sirkel-rebuild"));
    app.setDesktopFileName(QStringLiteral("io.github.strandzen.sirkel.settings"));
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("Rebuilds Sirkel after a KWin update, or tells you when it needs one."));
    parser.addHelpOption();
    const QCommandLineOption checkOption(QStringLiteral("check"), i18n("Show a notification if Sirkel needs a rebuild (run at login)."));
    parser.addOption(checkOption);
    parser.process(app);

    if (parser.isSet(checkOption)) {
        notifyRebuildNeeded(builtFor, installed);
    } else {
        startRebuild();
    }
    return app.exec();
}
