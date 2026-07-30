# SwordFM

Thunar-like Qt6 file manager with a sworddeck One Dark theme, Places sidebar, rclone device mounts, in-app preview panel, and XDG Open With support.

## Features

- Details + icon views, Places / Devices / Bookmarks
- Hides junk mounts (`/run/user/1000`, libvirt, systemd unit clutter)
- Shows **rclone** cloud mounts under Devices
- Right-panel preview for text, code, and images
- Double-click opens with the default app (mpv/vlc for video, etc.)
- Right-click **Open With** menu (Thunar-style)
- Copy / cut / paste, search filter, hidden files toggle

## Build

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Or:

```bash
./install.sh
```

## Run

```bash
./build/swordfm
./build/swordfm ~
```

## Dependencies

- Qt6 (Widgets, Core)
- CMake 3.20+
- C++20 compiler

## Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+L` | Focus path bar |
| `Alt+←/→` | Back / Forward |
| `Alt+↑` / `Backspace` | Up |
| `F5` | Refresh |
| `Ctrl+H` | Toggle hidden |
| `F2` | Rename |
| `Delete` | Delete |
| `Space` / `F3` | Preview |
| `F4` | Terminal here |
| `Ctrl+1` / `Ctrl+2` | Details / Icons |
