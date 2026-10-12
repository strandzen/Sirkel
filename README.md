# Sirkel

A [Loop](https://github.com/MrKai77/Loop)-inspired directional window snapping tool for KDE Plasma 6.7+ / KWin.

Hold a hyperkey (Caps Lock, remapped to F24), drag the mouse or press a direction key to pick where the active window should go, release to snap it there - left/right halves, top/bottom halves, or one of the four quarter corners, previewed live with an on-screen outline and a direction indicator.

## Features

- Hold-and-drag or hold-and-press-a-key directional snapping to 8 positions (halves + corners), with live outline and indicator previews
- Direction selection is sticky - once picked, relaxing the mouse or releasing the keys doesn't cancel it; only picking a different direction changes it
- Holding the hyperkey alone does nothing - the indicator only appears once you actually move the mouse or press a direction key, so a reflexive tap never triggers anything
- Resting the cursor at the indicator's own center maximizes the window instead - a live zone just like the other 8 directions, with its own padding applied
- Arrow keys, WASD, or HJKL for direction (configurable preset), including combining two keys (e.g. Up+Left) for a corner - or disable direction keys entirely for mouse-only use
- Hyperkey+Enter maximizes, Hyperkey+Backspace minimizes
- Esc, or releasing with no direction chosen, cancels cleanly
- Configurable hyperkey trigger: instant (default) or a hold delay
- Optional scroll-to-resize: while a direction is picked and previewing, the mouse wheel cycles the target through a small/normal/large size before you release - edges and corners resize only the one dimension they already control (the other stays full), Maximize shrinks width only, centered (height stays full)
- Snap to whichever monitor the cursor is on instead of always the active window's own screen (opt-in)
- Independent colors for the outline, the indicator's pointer, and the indicator's ring, each sourced from a manual pick, the live Plasma accent color, or (outline/pointer only) the current wallpaper - with a "linked" toggle to share one color across all three
- Optional blur behind the outline's translucent fill, using KWin's own Blur effect
- A handful of motion/feedback touches: configurable easing (linear/smooth/overshoot), a pop-in entrance, a confirm flash on release, a gradient fill option, brightness feedback as a drag commits, and a breathing pulse while resting at Maximize
- Tabbed settings page (System Settings → Desktop Effects → Sirkel): Input, Visuals, and Colors each on their own tab

## Requirements

- KDE Plasma 6.7+ (KWin with the Wayland session)
- [keyd](https://github.com/rvaiya/keyd) - hard dependency, used to remap Caps Lock to F24 at the input level
- Build dependencies: CMake, Extra CMake Modules (ECM), Qt6 (Core, Gui, Qml, Quick, DBus), KDE Frameworks 6 (Config, CoreAddons, GlobalAccel, KCMUtils), and KWin's own development headers

## Recommended
- [Geometry Change](https://github.com/peterfajdiga/kwin4_effect_geometry_change) - For fancier animations when windows change positons.

## Installation

### 1. Configure keyd

Create `/etc/keyd/default.conf`:

```ini
[ids]

*

[main]

capslock = f24
```

Then enable and start the service:

```sh
sudo systemctl enable --now keyd
```

This dedicates Caps Lock entirely to acting as the hyperkey.

### 2. Build and install the plugin

```sh
mkdir build && cd build
cmake ..
cmake --build .
sudo cmake --install .
```

This installs both the KWin effect itself and its System Settings configuration page.

### 3. Enable the effect

Open **System Settings → Desktop Effects**, find **Sirkel**, and enable it.

## Usage

1. Hold Caps Lock (F24).
2. Move the mouse toward an edge or corner, or press an arrow key (or WASD/HJKL, depending on the configured scheme) - the direction indicator and a preview outline show the current pick.
3. Release Caps Lock to snap the active window there, on its own screen (windows never move across monitors).

While held:
- **Enter**, or resting the cursor at the indicator's own center - maximize
- **Backspace** - minimize
- **Esc**, or releasing with no direction picked - cancel without moving the window

## Configuration

All settings live in **System Settings → Desktop Effects → Sirkel** (the configure/gear icon next to the effect), across three tabs:
- **Input** - mouse deadzone (horizontal/vertical), window padding, snap-to-cursor's-monitor toggle, keybind scheme (arrows/WASD/HJKL/none), hyperkey trigger mode (instant/delay), scroll-to-resize toggle
- **Visuals** - indicator and outline, each in their own section (visibility, size/thickness/radius, opacity, text label, blur), plus a shared Motion section (glide/grow animation speed, easing style, confirm flash, gradient fill)
- **Colors** - independent source per target (outline, pointer, ring): a manual color (hex field or R/G/B/A sliders), the live theme accent color, or (outline/pointer only) a wallpaper-derived color - with a "linked" toggle to share one color across all three, and a reset button scoped to just this tab

Settings changes take effect automatically the next time you hold the hyperkey - no need to log out or restart the effect.

## Known limitations

- Only one window is placed at a time - no multi-window "thirds" layout that's aware of its neighbors; scroll-to-resize lets you build one by hand (e.g. Left at its smaller step + Maximize at its smaller step, centered + Right at its smaller step), but each window is still placed independently
- Keybind schemes are fixed presets (arrows / WASD / HJKL / none), not arbitrary rebinding
- No dedicated action to move a window *to* a different monitor - "snap to cursor's monitor" changes which screen a snap targets, but there's no equivalent of Loop's screen-switching gesture yet

## Comparison with Loop

How Sirkel stacks up against [Loop](https://github.com/MrKai77/Loop), the macOS app it's heavily inspired by.

| Feature | Loop | Sirkel |
|---|---|---|
| Radial menu (hold key, drag mouse, release to snap) | ✅ | ✅ |
| Preview window before committing | ✅ | ✅ |
| Modifier + arrow keys | ✅ | ✅ |
| Modifier + mouse | ✅ | ✅ |
| Halves & quarters | ✅ | ✅ |
| Padding/margins | ✅ | ✅ |
| Radial menu theming (width/shape/color) | ✅ | ✅ |
| Preview theming (padding, corner radius, border color/width) | ✅ | ✅ |
| Indicator/preview independently toggleable | ✅ | ✅ |
| Maximize / center gesture | ✅ | ✅ |
| Separate colors for indicator vs. outline | ✅ | ✅ |
| Thirds (horizontal/vertical, two-thirds variants) | ✅ | ❌ |
| Per-action custom keybinds (any key → any specific action) | ✅ | ❌ |
| Cycles (cycles through size variants before committing) | ✅ | ✅ |
| Stash (hide windows at screen edge, reveal on hover) | ✅ | ❌ |
| Granular manipulation (Larger/Smaller, Grow/Shrink per edge, nudge) | ✅ | ❌ |
| Undo last action | ✅ | ❌ |
| Restore initial frame (snap back to pre-Sirkel geometry) | ✅ | ❌ |
| Screen switching (move window to an adjacent monitor) | ✅ | ❌ |
| Almost Maximize / Centre as distinct actions | ✅ | ❌ |
| Snap via drag-to-edge (no hyperkey needed) | ✅ | ❌ |

## Roadmap

- **Custom keybinds** - real per-action global shortcuts via `System Settings → Shortcuts` (maximize, minimize, each directional snap, and more), replacing the current fixed arrows/WASD/HJKL preset dropdown

## License

GPL-2.0-or-later.
