# Changelog

All notable changes to Sirkel. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses [semantic versioning](https://semver.org/). Before 1.0, settings may still change between versions.

## [Unreleased]

### Added
- A notification at login when Sirkel needs a rebuild after a KWin update, so it no longer just stops working without a word. It can be turned off in System Settings → Notifications or Autostart.
- A **Rebuild** button, in that notification and on the About page, for Sirkel installed as a package: it opens your default terminal (Konsole if none is set), which builds and installs it again with `makepkg -si` and closes when you press Enter. The package now carries the PKGBUILD it was built from for this. The button names the KWin version it rebuilds for, and says so first if the build tools (`base-devel`) are missing. The rebuild check reads the installed Sirkel itself, so the About page is right straight after a rebuild.
- The output of the last rebuild is saved in `~/.cache/sirkel/rebuild.log`, and the About page can show it, to attach to a bug report if a rebuild fails.
- The General and About pages check again when you come back to the settings window, so they show the new state after a rebuild (or after setting up keyd) without reopening it.
- Press R while holding the trigger to put the window back where it was before Sirkel placed it. Also as a direct shortcut (**Restore size**, not set out of the box).
- Click while holding the trigger to snap right away; right-click cancels.
- Software centers like Discover can list Sirkel (AppStream metadata).

### Changed
- The Shortcuts page no longer lists the direct shortcuts (snap straight to a half or quarter). They're set under Sirkel in System Settings → Shortcuts, which the page links to. **Defaults** leaves them alone.
- Settings now wait for **Apply** instead of applying as you change them, shortcuts included. The preview still shows every change straight away, and closing the window with unapplied changes asks whether to apply or discard them. **Defaults** fills in the defaults (no trigger, Enter/Backspace/R while holding), also applied only with Apply.
- The pacman hook says to rebuild right away: Sirkel keeps working until you log out after a KWin update, so rebuilding before that means it never stops working.
- Sirkel now also needs KNotifications, KIO and KService. Building without the rebuild notification and button: `-DBUILD_REBUILD_HELPER=OFF`.
- Trigger + Enter now picks maximize like a direction: the outline shows the full screen and the window is maximized when you let go of the trigger, so you can still pick something else or cancel. Backspace (minimize) still acts at once.
- After Esc, Backspace or R, keys you press while still holding the trigger stay with Sirkel until you let go, instead of reaching the app.
- Building needs Qt 6.10 and KDE Frameworks 6.26, what KWin 6.7 needs itself.
- Faster: checking whether Sirkel needs a rebuild no longer starts `kwin_wayland` (65 ms each time) or a full app at login, the settings read all shortcuts in one go, and a trigger press no longer re-reads the shortcut and color settings unless they changed.
- The settings' live preview follows Plasma's animation speed, like the real overlay.
- The About page is tidier: a larger (128 px), centered icon and version, one message saying which KWin Sirkel is built for and whether it runs, and the keyd steps (with a copy button) only until keyd is set up, recommended whatever the trigger. Once it is, a single "keyd is set up" line.

### Fixed
- With little or no ring roundness, the pointer and the Maximize fill cut diagonally across the corners. They now follow square corners exactly and match a slightly rounded ring.
- With "move the window under the mouse", an excluded app's window under the mouse (for example a desktop shell's window over the desktop) made Sirkel refuse to start. Excluded windows are now looked through, like panels.
- Running `makepkg -si` again in a folder used for an earlier build, after a KWin update, could produce a package still built for the old KWin (the new KWin's files looked older than the leftover build). The PKGBUILDs now always build from scratch.
- A window tiled by KWin (Meta+arrow quick tiling or custom tiles) stayed in its KWin tile after Sirkel moved it.
- Windows side by side (for example three thirds) could end up 1 px apart or overlapping. Edges are now whole pixels, shared exactly by neighbours.

## [0.5.3] - 2026-10-26

### Added
- Sirkel has its own icon, used by the launcher, the settings window, Desktop Effects and the About page.
- Dragging a window Sirkel placed gives it back the size it had before, like Plasma's own tiling. A cancelled drag (Esc) leaves it in place.
- The stress test checks where windows end up, that Enter and Backspace still reach apps outside a session, and that nothing works on the lock screen. It also runs weekly on GitHub against the latest KWin.
- A Troubleshooting section in the README.
- While holding the trigger, pressing the direction key that's already picked again changes the size: 1/2, 1/3, 1/4, 2/3 and round again (left, right, top and bottom).

### Changed
- The development PKGBUILD (`sirkel-git`) moved from the repository root to `packaging/sirkel-git`.
- The README says that no key combination is set out of the box: without keyd, a trigger has to be set in Sirkel's settings first.
- The trigger status on the General page names the trigger that's set (for example "Meta+Z is set as the trigger"), and turns orange when nothing would start Sirkel: no trigger set, or Caps Lock without keyd.
- Picking the direction opposite the current one (Left to Right) moves the pointer straight there instead of sweeping round half the ring.
- The rule for which key release ends a session is now covered by tests, and the stress test includes a key-combination trigger (Meta+Space).

### Fixed
- Windows with a minimum or maximum size could end up partly off-screen, or somewhere other than the preview showed. They're now centered on the spot you picked and kept on screen.

## [0.5.2] - 2026-10-26

### Added
- Sirkel can be translated. See `po/README.md` for how to add a language.
- Debug logging, which you can turn on in KDebugSettings (`sirkel` and `sirkel.settings`).
- Automatic builds and tests on GitHub, including a weekly build against the latest KWin.
- The keys for maximize (Enter) and minimize (Backspace) while holding the trigger can be changed, on the Shortcuts page or in System Settings → Shortcuts.
- Norwegian (Bokmål) translation.
- A pacman hook in both PKGBUILDs that reminds you to rebuild Sirkel when KWin is upgraded.
- A PKGBUILD for the latest release (`packaging/sirkel`), next to the development one.
- Links to the project page and bug tracker in System Settings → Desktop Effects.
- A stress test (`scripts/stress/`) that runs Sirkel in a headless KWin with simulated input, optionally with memory error checking.

### Changed
- Sirkel's shortcuts have their own **Sirkel** group in System Settings → Shortcuts instead of being listed under KWin. Existing bindings move over automatically.
- Auto-cancel is on by default (10 seconds), so a missed key release can't keep the keyboard grabbed.
- Sirkel never starts on the lock screen.
- Both PKGBUILDs run the tests while building and also build on aarch64.
- The ring now follows the Plasma accent by default, in a dark, muted version.
- One deadzone setting instead of separate horizontal and vertical ones.
- The pointer's length follows the indicator size.
- The shadow under the indicator and outline is always on.
- Colors are picked with Plasma's standard color dialog.
- The preview gets the wallpaper from Plasma directly.
- The settings launcher is now `io.github.strandzen.sirkel.settings.desktop`.
- The README leads with a key combination as the trigger; Caps Lock with keyd is optional.

### Removed
- The direction label inside the indicator.
- The "start after holding briefly" trigger option. Sirkel always starts immediately.
- The Two-tone and Shadow switches.
- Colors from the wallpaper, and the wallpaper color file setting.

### Fixed
- The PKGBUILD failed on systems without `vulkan-headers` installed.

## [0.5.1] - 2026-10-26

### Added
- The About page shows whether Sirkel is running. When the installed KWin differs from the version Sirkel was built for, it explains how to rebuild.

### Changed
- The settings window follows Plasma accent color changes while it's open.
- Ring thickness is capped at half the indicator size.
- The General page's "Hyperkey" section is now called "Trigger".
- Shortcuts are no longer looked up over D-Bus on every trigger press.
- README rewritten in plainer language, with a warning to rebuild after every Plasma update.

### Fixed
- A deadzone of 0 made every mouse movement pick a corner. The minimum is now 1 px.
- The indicator's first pick in a session slid over from where the previous session ended.
- The settings preview kept animating while hidden.
- Excluded apps could be added twice with different capitalization.
- A Two-tone tooltip still mentioned the removed gradient fill.

## [0.5.0] - 2026-10-20

### Added
- New settings window with a sidebar: General, Appearance, Colors, Shortcuts and About. Less common options sit in Advanced folds.
- "Sirkel" launcher entry that opens the settings in their own window.
- Live preview on the Appearance and Colors pages: the real indicator and outline at real size over the blurred wallpaper. The pane slides out next to the settings and can be resized or hidden.
- Key combination trigger as an alternative to Caps Lock. Releasing any key of the combination snaps.
- Direct shortcuts for every placement, maximize and minimize. None are set by default; they also show up under KWin in System Settings → Shortcuts.
- Angle picking mode: eight equal slices around a center radius, as an alternative to the per-axis box mode.
- Option to act on the window under the cursor instead of the active window.
- Excluded apps list.
- Auto-cancel after a set idle time while the trigger is held.
- Fluid easing (new default), shadow, two-tone shading and hairlines on the ring.
- Indicator animations: settle-in on appear, the reverse on exit, and a press-in when maximize is picked.
- Old settings are migrated automatically on the next login after an update.
- About page with version, keyd setup and status.

### Changed
- keyd is now optional.
- A theme-colored ring uses a dark, muted version of the accent.
- The outline, pointer and ring always have their own color settings.
- Enter now also focuses a window picked under the cursor.

### Removed
- Outline gradient fill.
- Confirm flash when the window snaps.
- "Use separate colors" toggle.

### Fixed
- Picking a direction with the cursor pinned at a screen edge. Movement past the edge now still counts.
- Sirkel no longer starts while Overview or another full-screen effect is open.

## [0.2.0] - 2026-10-18

### Added
- PKGBUILD for Arch-based systems (`sirkel-git`).
- Windows that can't be resized are moved and centered in the picked area.
- Fullscreen windows (games, video) are left alone.
- Unit tests for placement and direction picking.

### Changed
- Internals rewritten into separate parts for placement, input, trigger, overlay and colors.
- Settings apply right away through KWin instead of being re-read on every press.
- The indicator is kept fully on screen when it appears near an edge.
- Indicator text scales with the indicator, uses the system font and puts corner labels on two lines.

### Removed
- Breathing pulse while maximize is selected.

### Fixed
- Blur was missing on the first pick of a session.

## [0.1.0] - 2026-10-14

First versioned release.

### Added
- Hold Caps Lock (F24 via keyd), pick a direction with the mouse or keys, release to snap the active window.
- Halves, quarters and maximize. Moving back to the indicator's center picks maximize.
- Direction keys: arrows, WASD, HJKL or none. Two keys at once pick a corner, and releasing one of them keeps the corner.
- Picks stay selected until a new direction is picked.
- Holding Caps Lock alone does nothing until the mouse moves or a key is pressed.
- Enter maximizes, Backspace minimizes, Esc cancels.
- Scroll to resize before releasing: edges and corners cycle 1/4, 1/3, 1/2, 2/3; maximize cycles width 1/3, 1/2, full.
- Ring indicator with a curved pointer that slides along the ring. Size, thickness, roundness, pointer length, opacity and an optional direction label.
- Indicator at the screen center or at the cursor.
- Outline preview of the target with border, roundness, fill opacity, optional blur and a glide animation with adjustable duration and easing.
- Horizontal and vertical padding, with half padding between neighbouring windows.
- Option to snap on the monitor under the cursor.
- Trigger start: immediately or after holding briefly.
- Colors for outline, pointer and ring: custom, Plasma accent or a wallpaper color (pywal or a hex color file). Color picker with hex and alpha.
- Settings page in System Settings → Desktop Effects, with a warning when keyd isn't set up.
- Desktop, panels and other special windows are ignored.
- GPL-2.0-or-later license.

[0.5.3]: https://github.com/strandzen/Sirkel/compare/v0.5.2...v0.5.3
[0.5.2]: https://github.com/strandzen/Sirkel/compare/v0.5.1...v0.5.2
[0.5.1]: https://github.com/strandzen/Sirkel/compare/v0.5.0...v0.5.1
[0.5.0]: https://github.com/strandzen/Sirkel/compare/v0.2.0...v0.5.0
[0.2.0]: https://github.com/strandzen/Sirkel/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/strandzen/Sirkel/releases/tag/v0.1.0
