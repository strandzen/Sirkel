// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

// KWin only loads an effect built for its exact version, so Sirkel stops
// working after every KWin update until it's rebuilt. Shared by the
// settings' About page and sirkel-rebuild (the login notification and the
// Rebuild button), so both always agree. Doesn't link KWin: it has to keep
// working after the KWin update it checks for. Everything here reads files
// rather than starting programs, so it's cheap enough for every login.
namespace Rebuild
{

enum class EffectStatus {
    Running, // loaded, versions match
    Rebuild, // built for another KWin than the installed one
    Relogin, // enabled and matching, but not loaded yet
    Disabled, // turned off in Desktop Effects
};

// The KWin version in an effect plugin's IID
// ("org.kde.kwin.EffectPluginFactory6.8.1"): exactly what KWin compares
// before loading it. Empty for any other IID.
QString versionFromIid(const QString &iid);
// The KWin version a library link like libkwin.so.6 points at
// ("libkwin.so.6.8.1"); empty if there's no such file.
QString versionFromLibraryLink(const QString &link);

// The KWin version the installed Sirkel was built for, read from the
// installed plugin without loading it. Right even after a rebuild while
// the settings are open, unlike a version compiled into them. The version
// this binary was built against if the plugin can't be found.
QString builtForKWin();
// The installed KWin, which is what the next login runs.
QString installedKWin();
// Whether Sirkel is turned on in Desktop Effects (kwinrc, read fresh).
bool effectEnabled();

// The installed KWin is known and isn't the one Sirkel was built for.
bool needsRebuild(const QString &builtFor, const QString &installed);
EffectStatus effectStatus(const QString &builtFor, const QString &installed, bool loaded, bool enabled);
// "running", "rebuild", "relogin" or "disabled", for QML.
QString statusName(EffectStatus status);
// The login notification: only when Sirkel is turned on and won't load.
bool shouldNotify(const QString &builtFor, const QString &installed, bool enabled);

// Where a package install keeps the PKGBUILD it was built from (and the
// files it needs). Only packages install it: its presence is what tells a
// package install from a `cmake --install`.
QString packageDir();
// A PKGBUILD in packageDir() and sirkel-rebuild built.
bool canRebuild();
// What base-devel brings that makepkg and the build need: fakeroot,
// binutils, make and a C++ compiler. Looked for directly rather than asking
// pacman for base-devel, which systems that got it as a group (before it
// became a package) don't have although every tool is there.
bool hasBuildTools();

// ~/.cache/sirkel: the build folder, the lock and the log.
struct RebuildPaths {
    QString packageDir;
    QString buildDir;
    QString lockFile;
    QString logFile;
    QString scriptFile;
};
RebuildPaths defaultPaths();

// `value` single-quoted for sh.
QString shellQuote(const QString &value);

// The script the terminal runs: takes the lock (or says a rebuild is
// already running), copies the PKGBUILD into a clean build folder, runs
// `makepkg -si` with its output also saved to the log, says how it went and
// waits for Enter so the window stays open.
QString rebuildScript(const RebuildPaths &paths);

} // namespace Rebuild
