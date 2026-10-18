# Sirkel

Directional window snapping for KDE Plasma 6.7+ (Wayland), inspired by [Loop](https://github.com/MrKai77/Loop) for macOS.

Hold Caps Lock, move the mouse or press a direction key, release. The active window snaps to that half, quarter or to maximized. A ring indicator and an outline preview show where it will land.

Version 0.2.0. Early and tested on one machine, so settings may still change.

## Demo
[demo-video.webm](https://github.com/user-attachments/assets/7f5a1d62-6c34-4b2f-83e8-b8504662fea3)


## Features

- 8 placements (halves and quarters) plus maximize, picked with the mouse or with arrow keys, WASD or HJKL. Two keys at once (Up+Left) pick a corner.
- Holding Caps Lock alone does nothing. The indicator appears on the first mouse move or key press.
- Optional scroll-to-resize before releasing: edges and corners cycle 1/4, 1/3, 1/2, 2/3; maximize cycles 1/3, 1/2, full width.
- Padding around snapped windows, with half padding between neighbours so gaps stay even.
- Windows that can't be resized are moved and centered in the picked area. Fullscreen windows (games, video) are left alone.
- Colors for the outline, pointer and ring: manual, Plasma accent, or a wallpaper color (pywal or any file with a hex color).
- Optional blur behind the outline, gradient fill, adjustable animation speed and easing.

## Requirements

- KDE Plasma 6.7+ on Wayland
- [keyd](https://github.com/rvaiya/keyd), to turn Caps Lock into F24
- To build: CMake, Extra CMake Modules, Qt 6 (Core, Gui, Qml, Quick, DBus, Test), KDE Frameworks 6 (Config, ColorScheme, CoreAddons, GlobalAccel, I18n, KCMUtils, WindowSystem) and the KWin development headers

Optional: [Geometry Change](https://github.com/peterfajdiga/kwin4_effect_geometry_change) animates the window itself as it adjusts size.

## Installation

### 1. Set up keyd

Add this to `/etc/keyd/default.conf` (Sirkel never writes there itself):

```ini
[ids]
*

[main]
capslock = f24
```

```sh
sudo systemctl enable --now keyd
```

Caps Lock now works only as the hyperkey.

### 2. Build and install

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

### 3. Enable

**System Settings → Desktop Effects → Sirkel**. If keyd isn't running or Caps Lock isn't mapped to F24, Sirkel's settings page shows a warning.

## Usage

1. Hold Caps Lock.
2. Move the mouse toward an edge or corner, or press a direction key. Leave the cursor at the center of the indicator to maximize.
3. Release Caps Lock to snap.

While holding Caps Lock:

| Key | Action |
|---|---|
| Enter | Maximize |
| Backspace | Minimize |
| Esc | Cancel |
| Scroll wheel | Resize the target (if enabled) |

Windows snap on their own screen. With "Snap to cursor's monitor" on, they snap on the screen the cursor is on instead.

## Settings

**System Settings → Desktop Effects → Sirkel** (gear icon). Changes apply right away.

- **Input:** deadzones, padding, monitor targeting, direction keys, trigger (instant or after a hold delay), scroll-to-resize
- **Visuals:** indicator and outline size, shape, opacity and visibility, blur, flash, gradient, animation speed and easing
- **Colors:** source and color for outline, pointer and ring, or one linked color for all three

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
| Per-action custom shortcuts | ✅ | ❌ |
| Move window to another monitor | ✅ | ❌ |
| Undo / restore original size | ✅ | ❌ |
| Stash, nudge, grow/shrink per edge | ✅ | ❌ |
| Snap by dragging to a screen edge | ✅ | ❌ |

## Planned

- Fill empty gaps: hold Shift (or other key) while picking to fill a free space between existing windows
- Fully bindable keys in System Settings → Shortcuts

## License

GPL-2.0-or-later. See [LICENSE](LICENSE).
