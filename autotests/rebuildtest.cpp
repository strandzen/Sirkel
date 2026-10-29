// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rebuildstatus.h"

#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

using Rebuild::EffectStatus;

class RebuildTest : public QObject
{
    Q_OBJECT

private:
    // A package folder with a PKGBUILD, and a fake makepkg on PATH that
    // prints its arguments and the folder it runs in, then exits with
    // $FAKE_STATUS.
    struct Setup {
        QTemporaryDir dir;
        Rebuild::RebuildPaths paths;
        QString binDir;
    };
    static void prepare(Setup &setup)
    {
        QVERIFY(setup.dir.isValid());
        // A space and a quote in the path: everything must be quoted.
        const QString root = setup.dir.path() + QStringLiteral("/it's here");
        QVERIFY(QDir().mkpath(root + QStringLiteral("/package")));
        QVERIFY(QDir().mkpath(root + QStringLiteral("/bin")));
        writeFile(root + QStringLiteral("/package/PKGBUILD"), "pkgname=sirkel\n");
        writeFile(root + QStringLiteral("/package/sirkel-rebuild.hook"), "[Trigger]\n");
        writeFile(root + QStringLiteral("/bin/makepkg"), "#!/bin/sh\necho \"fake makepkg $* in $(basename \"$PWD\")\"\nls\nexit \"${FAKE_STATUS:-0}\"\n");
        QVERIFY(QFile::setPermissions(root + QStringLiteral("/bin/makepkg"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        setup.binDir = root + QStringLiteral("/bin");
        setup.paths = {
            .packageDir = root + QStringLiteral("/package"),
            .buildDir = root + QStringLiteral("/cache/rebuild"),
            .lockFile = root + QStringLiteral("/cache/rebuild.lock"),
            .logFile = root + QStringLiteral("/cache/rebuild.log"),
            .scriptFile = root + QStringLiteral("/cache/rebuild.sh"),
        };
        QVERIFY(QDir().mkpath(root + QStringLiteral("/cache")));
        writeFile(setup.paths.scriptFile, Rebuild::rebuildScript(setup.paths).toUtf8());
    }
    static void writeFile(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
    // Runs the script as the terminal would, pressing Enter at the end.
    static QString run(const Setup &setup, int fakeStatus = 0)
    {
        QProcess process;
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("PATH"), setup.binDir + QLatin1Char(':') + environment.value(QStringLiteral("PATH")));
        environment.insert(QStringLiteral("FAKE_STATUS"), QString::number(fakeStatus));
        process.setProcessEnvironment(environment);
        process.start(QStringLiteral("sh"), {setup.paths.scriptFile});
        process.write("\n");
        process.closeWriteChannel();
        if (!process.waitForFinished(20000)) {
            return QStringLiteral("(timed out)");
        }
        return QString::fromUtf8(process.readAllStandardOutput() + process.readAllStandardError());
    }
    static QString readFile(const QString &path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }
    static bool haveScriptTools()
    {
        return !QStandardPaths::findExecutable(QStringLiteral("script")).isEmpty() && !QStandardPaths::findExecutable(QStringLiteral("flock")).isEmpty();
    }

private Q_SLOTS:
    void status_data()
    {
        QTest::addColumn<QString>("installed");
        QTest::addColumn<bool>("loaded");
        QTest::addColumn<bool>("enabled");
        QTest::addColumn<EffectStatus>("status");
        QTest::addColumn<bool>("notify");

        QTest::newRow("running") << QStringLiteral("6.8.1") << true << true << EffectStatus::Running << false;
        QTest::newRow("kwin updated") << QStringLiteral("6.8.2") << false << true << EffectStatus::Rebuild << true;
        // Updated but not logged out yet: the old KWin still runs Sirkel.
        QTest::newRow("updated, still loaded") << QStringLiteral("6.8.2") << true << true << EffectStatus::Rebuild << true;
        QTest::newRow("updated, turned off") << QStringLiteral("6.8.2") << false << false << EffectStatus::Rebuild << false;
        QTest::newRow("just installed") << QStringLiteral("6.8.1") << false << true << EffectStatus::Relogin << false;
        QTest::newRow("turned off") << QStringLiteral("6.8.1") << false << false << EffectStatus::Disabled << false;
        // kwin_wayland couldn't be run: never ask for a rebuild.
        QTest::newRow("unknown version") << QString() << false << true << EffectStatus::Relogin << false;
    }
    void status()
    {
        QFETCH(QString, installed);
        QFETCH(bool, loaded);
        QFETCH(bool, enabled);
        QFETCH(EffectStatus, status);
        QFETCH(bool, notify);
        QCOMPARE(Rebuild::effectStatus(QStringLiteral("6.8.1"), installed, loaded, enabled), status);
        QCOMPARE(Rebuild::shouldNotify(QStringLiteral("6.8.1"), installed, enabled), notify);
    }

    void statusNames()
    {
        QCOMPARE(Rebuild::statusName(EffectStatus::Running), QStringLiteral("running"));
        QCOMPARE(Rebuild::statusName(EffectStatus::Rebuild), QStringLiteral("rebuild"));
        QCOMPARE(Rebuild::statusName(EffectStatus::Relogin), QStringLiteral("relogin"));
        QCOMPARE(Rebuild::statusName(EffectStatus::Disabled), QStringLiteral("disabled"));
    }

    void versionFromIid()
    {
        QCOMPARE(Rebuild::versionFromIid(QStringLiteral("org.kde.kwin.EffectPluginFactory6.8.1")), QStringLiteral("6.8.1"));
        QCOMPARE(Rebuild::versionFromIid(QStringLiteral("org.kde.kwin.PluginFactoryInterface6.8.1")), QString());
        QCOMPARE(Rebuild::versionFromIid(QString()), QString());
    }

    // The installed KWin is where libkwin's soname link points.
    void versionFromLibraryLink()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        writeFile(dir.filePath(QStringLiteral("libkwin.so.6.8.2")), "");
        QVERIFY(QFile::link(dir.filePath(QStringLiteral("libkwin.so.6.8.2")), dir.filePath(QStringLiteral("libkwin.so.6"))));
        QCOMPARE(Rebuild::versionFromLibraryLink(dir.filePath(QStringLiteral("libkwin.so.6"))), QStringLiteral("6.8.2"));
        // A missing link (KWin not installed) is unknown, never a rebuild.
        QCOMPARE(Rebuild::versionFromLibraryLink(dir.filePath(QStringLiteral("libnothing.so.6"))), QString());
    }

    // What the build needs, looked for on PATH.
    void buildTools()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QByteArray path = qgetenv("PATH");
        qputenv("PATH", QFile::encodeName(dir.path()));
        const auto tool = [&dir](const char *name) {
            const QString file = dir.filePath(QString::fromLatin1(name));
            writeFile(file, "#!/bin/sh\n");
            QFile::setPermissions(file, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        };
        tool("fakeroot");
        tool("strip");
        tool("make");
        QVERIFY(!Rebuild::hasBuildTools());
        tool("c++");
        QVERIFY(Rebuild::hasBuildTools());
        qputenv("PATH", path);
    }

    void shellQuote()
    {
        for (const QString &value : {QStringLiteral("plain"), QStringLiteral("with space"), QStringLiteral("it's"), QStringLiteral("$HOME `x` \"q\"")}) {
            QProcess sh;
            sh.start(QStringLiteral("sh"), {QStringLiteral("-c"), QStringLiteral("printf '%s' ") + Rebuild::shellQuote(value)});
            QVERIFY(sh.waitForFinished());
            QCOMPARE(QString::fromUtf8(sh.readAllStandardOutput()), value);
        }
    }

    void scriptBuildsInACleanCopy()
    {
        if (!haveScriptTools()) {
            QSKIP("script or flock (util-linux) not installed");
        }
        Setup setup;
        prepare(setup);
        // Left over from an earlier build: must be gone.
        QVERIFY(QDir().mkpath(setup.paths.buildDir));
        writeFile(setup.paths.buildDir + QStringLiteral("/stale"), "old");

        const QString output = run(setup);
        QVERIFY2(output.contains(QStringLiteral("fake makepkg -si in rebuild")), qPrintable(output));
        QVERIFY2(output.contains(QStringLiteral("Sirkel is rebuilt")), qPrintable(output));
        QVERIFY(output.contains(QStringLiteral("Press Enter")));
        QVERIFY(QFile::exists(setup.paths.buildDir + QStringLiteral("/PKGBUILD")));
        QVERIFY(QFile::exists(setup.paths.buildDir + QStringLiteral("/sirkel-rebuild.hook")));
        QVERIFY(!QFile::exists(setup.paths.buildDir + QStringLiteral("/stale")));
        // The log has makepkg's output for a bug report.
        QVERIFY(readFile(setup.paths.logFile).contains(QStringLiteral("fake makepkg -si")));
    }

    void scriptReportsAFailure()
    {
        if (!haveScriptTools()) {
            QSKIP("script or flock (util-linux) not installed");
        }
        Setup setup;
        prepare(setup);
        const QString output = run(setup, 1);
        QVERIFY2(output.contains(QStringLiteral("didn't finish")), qPrintable(output));
        QVERIFY(output.contains(setup.paths.logFile));
        QVERIFY(!output.contains(QStringLiteral("Sirkel is rebuilt")));
    }

    void scriptRunsOnlyOnce()
    {
        if (!haveScriptTools()) {
            QSKIP("script or flock (util-linux) not installed");
        }
        Setup setup;
        prepare(setup);
        // Another rebuild holds the lock.
        const int fd = ::open(QFile::encodeName(setup.paths.lockFile).constData(), O_WRONLY | O_CREAT | O_CLOEXEC, 0600);
        QVERIFY(fd >= 0);
        QCOMPARE(::flock(fd, LOCK_EX | LOCK_NB), 0);
        const QString output = run(setup);
        ::close(fd);
        QVERIFY2(output.contains(QStringLiteral("already running")), qPrintable(output));
        QVERIFY(!output.contains(QStringLiteral("fake makepkg")));
        QVERIFY(!QFile::exists(setup.paths.buildDir));
    }
};

QTEST_GUILESS_MAIN(RebuildTest)

#include "rebuildtest.moc"
