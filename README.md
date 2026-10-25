# Sirkel

Window snapping for KDE Plasma 6.7+ on Wayland, inspired by [Loop](https://github.com/MrKai77/Loop) on macOS.

Hold Caps Lock, move the mouse toward where you want the window, and let go. It snaps to that half or quarter of the screen, or fills the screen if you leave the mouse in the middle. A small ring follows your direction and an outline shows where the window will end up, so you know what you'll get before you let go.

This is version 0.5.1. It's early and I've only tested it on my own machine, so settings may still change.

> [!WARNING]
> You have to rebuild Sirkel after every Plasma update, even small ones like 6.8.0 to 6.8.1. KWin refuses to load effects that were built for a different version. See [Installation](#installation).

## Demo
[demo-video.webm](https://github.com/user-attachments/assets/7f5a1d62-6c34-4b2f-83e8-b8504662fea3)


## Features

- Snap to the left, right, top or bottom half, any of the four quarters, or full screen.
- Pick a direction with the mouse or the keyboard (arrow keys, WASD or HJKL). Press two keys together, like Up and Left, for a corner.
- Use Caps Lock as the trigger (with keyd), or any key combination you like, such as Meta+Space.
- Set your own shortcuts that snap straight away, without the ring. None are set out of the box.
- Scroll while holding the trigger to change the size: 1/4, 1/3, 1/2 or 2/3 of the screen. Full screen goes 1/3, 1/2, full width. This is off by default.
- Snap the active window or the one under the mouse, on its own monitor or the one the mouse is on.
- Add gaps around windows. Two windows next to each other get the same gap between them as against the screen edge.
- Windows that can't be resized are moved to the middle of the spot you picked. Fullscreen games and videos are left alone, and you can exclude apps.
- Colors can be your own, follow the Plasma accent color, or come from your wallpaper (pywal or any file with a hex color). If the ring follows the accent, it uses a darker, softer version of it so the pointer stands out.
- Optional blur, shadow and two-tone shading, with adjustable animation speed and style.
- A settings window with a live preview, so you can see changes as you make them.

## Requirements

- KDE Plasma 6.7 or newer, on Wayland
- [keyd](https://github.com/rvaiya/keyd), if you want to use Caps Lock as the trigger. You can skip it and use a key combination instead.
- To build it: CMake, Extra CMake Modules, Qt 6 (Core, Gui, Qml, Quick, DBus, Test, Widgets), KDE Frameworks 6 (Config, ColorScheme, CoreAddons, GlobalAccel, I18n, KCMUtils, WindowSystem) and the KWin development headers

[Geometry Change](https://github.com/peterfajdiga/kwin4_effect_geometry_change) works well alongside Sirkel if you want the window itself to animate into place.

## Installation

### 1. Build and install

```sh
cmake -B build
cmake --build build
sudo cmake --install build
```

On Arch and Arch-based systems you can build a package instead. Run this in the repository folder:

```sh
makepkg -si
```

Note that the PKGBUILD downloads and builds the latest version from GitHub, not the copy on your disk.

**After every Plasma update**, rebuild and reinstall, then log out and back in:

```sh
cmake --build build
sudo cmake --install build
```

If you installed with the PKGBUILD, just run `makepkg -si` again. The About page in Sirkel's settings tells you when a rebuild is needed.

### 2. Turn it on

Go to **System Settings → Desktop Effects** and enable **Sirkel**.

### 3. Choose a trigger

**Caps Lock (the default):** Sirkel listens for the F24 key, and keyd turns Caps Lock into F24. Add this to `/etc/keyd/default.conf` (Sirkel never touches that file itself):

```ini
[ids]
*

[main]
capslock = f24
```

Then start keyd:

```sh
sudo systemctl enable --now keyd
```

Caps Lock won't work as Caps Lock anymore after this. The About page in Sirkel's settings shows whether keyd is set up correctly.

**A key combination:** open Sirkel's settings, go to Shortcuts and record the combination you want.

## Usage

1. Hold the trigger.
2. Move the mouse toward an edge or corner, or press a direction key. Keep the mouse near the middle of the ring to fill the screen.
3. Let go to snap.

While you hold the trigger, you can also press:

| Key | What it does |
|---|---|
| Enter | Fill the screen |
| Backspace | Minimize |
| Esc | Cancel |
| Scroll wheel | Change the size (if turned on) |

## Settings

You'll find them under **System Settings → Desktop Effects → Sirkel**, or by searching for **Sirkel** in the app launcher. Changes apply right away.

- **General:** how you pick a direction, which keys to use, scrolling to resize, which window and monitor to snap, gaps, the trigger delay and excluded apps
- **Appearance:** size, shape and look of the ring and the outline, plus animation speed and style
- **Colors:** colors for the outline, the pointer and the ring
- **Shortcuts:** the trigger and your own snap shortcuts

The Appearance and Colors pages have a preview on the right. Drag the window edge to make it bigger, or hide it with the button at the top.

Every shortcut also shows up under KWin in **System Settings → Shortcuts**.

## Uninstall

In the source folder, run:

```sh
sudo xargs rm < build/install_manifest.txt
```

If you used the package, run `sudo pacman -R sirkel-git` instead. To get Caps Lock back, remove the `capslock = f24` line from the keyd config and run `sudo systemctl restart keyd`.

## Compared to Loop

| Feature | Loop | Sirkel |
|---|---|---|
| Radial menu with live preview | ✅ | ✅ |
| Pick with mouse or arrow keys | ✅ | ✅ |
| Halves, quarters, thirds and size cycling | ✅ | ✅ |
| Gaps, themes and separate colors | ✅ | ✅ |
| Custom shortcuts per action | ✅ | ✅ |
| Move a window to another monitor | ✅ | ❌ |
| Undo / restore the original size | ✅ | ❌ |
| Stash, nudge, grow or shrink one edge | ✅ | ❌ |
| Snap by dragging to a screen edge | ✅ | ❌ |

## Planned

- Animate the window move in Sirkel itself, and a live mode where the window moves as you pick
- Fill the gap: hold a key while picking to fill an empty space between your windows

## License

GPL-2.0-or-later. See [LICENSE](LICENSE).
