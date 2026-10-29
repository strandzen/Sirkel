// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <KSharedConfig>

#include <QDateTime>
#include <QKeySequence>
#include <QList>

class QAction;

// KGlobalAccel's own file, kglobalshortcutsrc, kept open by its owner (the
// effect) so it's parsed once rather than on every lookup:
// registerGlobalAction() and storedShortcuts() read the open copy. Re-read
// only when KGlobalAccel has saved it since, which costs one stat() per
// session instead of a parse.
class ShortcutsFile
{
public:
    ShortcutsFile();
    // Whether it was re-read (it changed since it was opened or last re-read).
    bool reloadIfChanged();

private:
    KSharedConfigPtr m_config;
    QDateTime m_modified;
};

// Registers one of Sirkel's actions with KGlobalAccel, under Sirkel's own
// group in System Settings -> Shortcuts (kShortcutComponent), with
// `defaults` as its default keys. A binding the user set while Sirkel's
// shortcuts still lived under KWin's group is carried over once, and the
// old KWin entry is removed.
void registerGlobalAction(QAction *action, const QList<QKeySequence> &defaults);

// The keys stored for `actionId`, read from KGlobalAccel's own file rather
// than asked for over D-Bus (the service runs inside KWin, so a lookup would
// block the compositor), as last read by ShortcutsFile. `defaults` if
// nothing is stored yet.
QList<QKeySequence> storedShortcuts(const QString &actionId, const QList<QKeySequence> &defaults);
