# Sirkel

Directional window snapping for KDE Plasma 6.7+ (Wayland), inspired by [Loop](https://github.com/MrKai77/Loop) for macOS.

Hold Caps Lock (or a key combination of your choice), move the mouse or press a direction key, release. The window snaps to that half, quarter or to maximized. A ring indicator and an outline preview show where it will land.

Version 0.5.0. Early and tested on one machine, so settings may still change.

## Demo
[demo-video.webm](https://github.com/user-attachments/assets/7f5a1d62-6c34-4b2f-83e8-b8504662fea3)


## Features

- 8 placements (halves and quarters) plus maximize, picked with the mouse or with arrow keys, WASD or HJKL. Two keys at once (Up+Left) pick a corner.
- Two ways to pick with the mouse: box (separate deadzone per axis) or angle (eight equal slices around a center radius).
- Trigger with Caps Lock via keyd, or with any key combination such as Meta+Space. Releasing any key of the combination snaps.
- Direct shortcuts for every placement, maximize and minimize, without the indicator. None are set by default.
- Optional scroll-to-resize before releasing: edges and corners cycle 1/4, 1/3, 1/2, 2/3; maximize cycles 1/3, 1/2, full width.
- Act on the active window or the window under the cursor, on its own monitor or the one under the cursor.
- Padding around snapped windows, with half padding between neighbours so gaps stay even.
- Windows that can't be resized are moved and centered in the picked area. Fullscreen windows (games, video) and excluded apps are left alone.
- Colors for the outline, pointer and ring: manual, Plasma accent, or a wallpaper color (pywal or any file with a hex color). A theme-colored ring uses a dark, muted version of the accent.
- Optional blur, shadow and two-tone shading. Adjustable animation duration and easing.
- Settings window with a live preview of the indicator and outline at real size.

## Requirements

- KDE Plasma 6.7+ on Wayland
- Optional: [keyd](https://github.com/rvaiya/keyd), to use Caps Lock as the trigger. Without it, set a key combination in the settings.
- To build: CMake, Extra CMake Modules, Qt 6 (Core, Gui, Qml, Quick, DBus, Test, Widgets), KDE Frameworks 6 (Config, ColorScheme, CoreAddons, GlobalAccel, I18n, KCMUtils, WindowSystem) and the KWin development headers

Optional: [Geometry Change](https://github.com/peterfajdiga/kwin4_effect_geometry_change) animates the window itself as it adjusts size.

## Installation

### 1. Build and install

```sh
cmake -B build
cmake --build build
sudo cmake --install build
```

On Arch-based systems you can build a package instead, from the repository root:

```sh
makepkg -si
```

The PKGBUILD builds the latest commit on GitHub, not your local copy.

### 2. Enable

**System Settings → Desktop Effects → Sirkel**.

### 3. Pick a trigger

**Caps Lock (default):** add this to `/etc/keyd/default.conf` (Sirkel never writes there itself):

```ini
[ids]
*

[main]
capslock = f24
```

```sh
sudo systemctl enable --now keyd
```

Caps Lock now works only as the trigger. The About page in Sirkel's settings shows whether keyd is set up.

**Key combination:** open Sirkel's settings, go to Shortcuts and choose a combination instead.

## Usage

1. Hold the trigger.
2. Move the mouse toward an edge or corner, or press a direction key. Leave the cursor at the center of the indicator to maximize.
3. Release to snap.

While holding the trigger:

| Key | Action |
|---|---|
| Enter | Maximize |
| Backspace | Minimize |
| Esc | Cancel |
| Scroll wheel | Resize the target (if enabled) |

## Settings

Open them from **System Settings → Desktop Effects → Sirkel** (gear icon), or from the **Sirkel** entry in the application launcher. Changes apply right away.

- **General:** picking mode, direction keys, scroll-to-resize, target window and monitor, gaps, deadzones, trigger delay, excluded apps
- **Appearance:** indicator and outline size, shape, opacity and visibility, blur, shadow, two-tone, animation duration and easing
- **Colors:** source and color for outline, pointer and ring
- **Shortcuts:** trigger (Caps Lock or a combination) and the direct shortcuts

Appearance and Colors have a preview pane on the right. Drag the window edge to resize it, or hide it from the header.

All shortcuts are also listed under KWin in **System Settings → Shortcuts**.

## Uninstall

From the source directory:

```sh
sudo xargs rm < build/install_manifest.txt
```

Or `sudo pacman -R sirkel-git` if you used the package. To undo the keyd setup, remove the `capslock = f24` line and run `sudo systemctl restart keyd`.

## Compared to Loop

| Feature | Loop | Sirkel |
|---|---|---|
| Radial menu with live preview | ✅ | ✅ |
| Mouse and arrow-key picking | ✅ | ✅ |
| Halves, quarters, thirds, cycling sizes | ✅ | ✅ |
| Padding, theming, separate colors | ✅ | ✅ |
| Per-action custom shortcuts | ✅ | ✅ |
| Move window to another monitor | ✅ | ❌ |
| Undo / restore original size | ✅ | ❌ |
| Stash, nudge, grow/shrink per edge | ✅ | ❌ |
| Snap by dragging to a screen edge | ✅ | ❌ |

## Planned

- Sirkel animates the window move itself, then a live mode where the window follows the pick
- Fill empty gaps: hold a key while picking to fill a free space between existing windows

## License

GPL-2.0-or-later. See [LICENSE](LICENSE).
