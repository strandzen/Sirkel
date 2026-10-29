// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// Loaded into the test KWin by stress-test.sh: prints every normal window's
// frame geometry, and whether it's in one of KWin's own tiles, to KWin's
// log. TOKEN is replaced per run, so each probe's lines can be told apart.
const windows = workspace.windowList();
for (let i = 0; i < windows.length; ++i) {
    const w = windows[i];
    if (!w.normalWindow) {
        continue;
    }
    const g = w.frameGeometry;
    print("SIRKEL-PROBE TOKEN " + w.caption + "|" + g.x + "|" + g.y + "|" + g.width + "|" + g.height + "|" + (w.tile ? "tiled" : "untiled"));
}
