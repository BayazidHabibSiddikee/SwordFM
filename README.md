# SwordFM

A Thunar-like file manager built from scratch in C++20 with Qt6. One Dark theme, rclone cloud mounts, live preview panel, and full keyboard navigation.

![SwordFM](screenshots/1785729392.png)

![SwordFM](screenshots/1785729404.png)

---

## Included Tools

Every release package bundles all four — no separate downloads.

| Tool | What it does |
|------|-------------|
| `swordfm` | The file manager |
| `swordshare` | Password-protected LAN file sharing with QR code |
| `swordgraph` | Visualize folder structure as an interactive graph |
| `swordconv` | Document conversion — PDF, DOCX, MD, TXT, HTML |

---

## Installation

### Option 1: .deb package (Ubuntu / Debian / Kali)

```bash
wget https://github.com/BayazidHabibSiddikee/SwordFM/releases/download/v1.0.0/swordfm-1.0.0-amd64.deb
sudo dpkg -i swordfm-1.0.0-amd64.deb
sudo apt-get install -f
swordfm
```

Installs `swordfm`, `swordshare`, `swordgraph`, and `swordconv` to `/usr/bin/`.

> **Qt version note:** The .deb was built against Qt 6.11. If you get a
> `version 'Qt_6.x' not found` error, your distro ships an older Qt6.
> Use Option 2 (tarball) or Option 3 (build from source) instead —
> both work with any Qt6 version.

---

### Option 2: Portable tarball (any Linux, recommended if .deb fails)

```bash
wget https://github.com/BayazidHabibSiddikee/SwordFM/releases/download/v1.0.0/swordfm-1.0.0-linux-x64.tar.gz
tar xzf swordfm-1.0.0-linux-x64.tar.gz
cd SwordFM
./install.sh        # installs to ~/.local/bin — no sudo needed
```

Or run directly without installing:

```bash
./swordfm
```

> **Same Qt version note applies here.** If the binary fails to launch with
> a Qt version error, use Option 3 to build from source.

---

### Option 3: Build from source (works on any Qt6 version)

This is the most reliable option — it compiles against whatever Qt6 your system has.

```bash
git clone https://github.com/BayazidHabibSiddikee/SwordFM.git
cd SwordFM

# Ubuntu / Debian / Kali
sudo apt install qt6-base-dev cmake g++ build-essential

# Arch
sudo pacman -S qt6-base cmake gcc

# Fedora
sudo dnf install qt6-qtbase-devel cmake gcc-c++

# Build and install
./install.sh
```

`install.sh` installs `swordfm` to `~/.local/bin/` and all three helpers automatically.

---

## After Installing — Required & Optional Packages

**Qt6 runtime** (required — the file manager won't launch without it):

```bash
# Ubuntu / Debian / Kali
sudo apt install libqt6widgets6

# Arch
sudo pacman -S qt6-base

# Fedora
sudo dnf install qt6-qtbase
```

**Python 3** (required — for swordshare, swordgraph, swordconv):

Pre-installed on most distros. Check with `python3 --version`.

```bash
# If missing:
sudo apt install python3
```

**qrcode** (optional — only needed for QR codes in swordshare):

`swordshare` works fine without it. The QR panel will just be blank.

```bash
pip install qrcode[pil]
# or system-wide:
sudo apt install python3-qrcode
```

---

## Features

- **Details + Icon views** — `Ctrl+1` / `Ctrl+2`
- **Places sidebar** — Home, Desktop, Documents, Downloads, Trash, bookmarks
- **Devices** — mounted drives, rclone cloud mounts
- **Preview panel** — text, code, images, markdown — `Space` / `F3`
- **Open With** — right-click to pick which app opens a file
- **Search** — filter files in current dir, or search recursively
- **Copy / Cut / Paste** — `Ctrl+C`, `Ctrl+X`, `Ctrl+V`
- **Multi-select** — `Ctrl+Click` or `Shift+Click`
- **Hidden files** — `Ctrl+H`
- **Rename** — `F2`
- **Delete** — sends to trash
- **Open terminal here** — `F4`
- **LAN file sharing** — right-click → Share → QR code + password
- **Folder graph** — right-click folder → View Graph
- **Document conversion** — right-click file → Convert To
- **One Dark theme**

---

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+L` | Focus path bar |
| `Alt+←` | Back |
| `Alt+→` | Forward |
| `Alt+↑` / `Backspace` | Go up |
| `F5` | Refresh |
| `Ctrl+H` | Toggle hidden files |
| `F2` | Rename |
| `Delete` | Send to trash |
| `Space` / `F3` | Toggle preview panel |
| `F4` | Open terminal here |
| `Ctrl+1` | Details view |
| `Ctrl+2` | Icon view |
| `Ctrl+C` / `X` / `V` | Copy / Cut / Paste |
| `Ctrl+A` | Select all |
| `Ctrl+N` | New folder |
| `Enter` | Open / enter directory |
| `Ctrl+Q` | Quit |

---

## Make It Permanent

```bash
# Set as default file manager
xdg-mime default swordfm.desktop inode/directory

# i3 keybind
bindsym $mod+e exec swordfm ~
```

Autostart (`~/.config/autostart/swordfm.desktop`):

```ini
[Desktop Entry]
Type=Application
Name=SwordFM
Exec=swordfm
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
```

---

## Uninstall

```bash
# If installed via .deb
sudo dpkg -r swordfm

# If installed via tarball / install.sh
cd SwordFM && ./uninstall.sh
```

---

## Architecture

```
src/
├── app/      Entry point, main window, theme
├── model/    Filesystem model, filter, search
├── view/     Details + icon view
├── panel/    Sidebar, toolbar, statusbar, preview
└── ops/      File ops, sharing, conversion, context menu

tools/
├── swordshare    LAN file sharing server (Python)
├── swordgraph    Folder graph visualizer (Python)
└── swordconv     Document converter (Python)
```

---

## Related Projects

- **[SwordDeck](https://github.com/BayazidHabibSiddikee/CyberPaper)** — Animated desktop HUD with mission graph, system stats, and audio visualizer

---

## License

MIT
