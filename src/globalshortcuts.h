// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QKeySequence>
#include <QList>

class QAction;

// Registers one of Sirkel's actions with KGlobalAccel, under Sirkel's own
// group in System Settings -> Shortcuts (kShortcutComponent), with
// `defaults` as its default keys. A binding the user set while Sirkel's
// shortcuts still lived under KWin's group is carried over once, and the
// old KWin entry is removed.
void registerGlobalAction(QAction *action, const QList<QKeySequence> &defaults);

// The keys stored for `actionId`, read from KGlobalAccel's own file rather
// than asked for over D-Bus (the service runs inside KWin, so a lookup would
// block the compositor). `defaults` if nothing is stored yet.
QList<QKeySequence> storedShortcuts(const QString &actionId, const QList<QKeySequence> &defaults);
