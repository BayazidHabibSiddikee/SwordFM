# SwordFM

A Thunar-like file manager built from scratch in C++20 with Qt6. One Dark theme, rclone cloud mounts, live preview panel, and full keyboard navigation.

![SwordFM](screenshots/1785729392.png)

![SwordFM](screenshots/1785729404.png)

---

## Features

- **Details + Icon views** — switch with `Ctrl+1` / `Ctrl+2`
- **Places sidebar** — Home, Desktop, Documents, Downloads, Trash, plus user bookmarks
- **Devices** — shows mounted drives, rclone cloud mounts, hides junk (`/run/user/1000`, libvirt, systemd)
- **Preview panel** — live preview for text, code, images, and markdown (`Space` / `F3`)
- **Open With** — right-click Thunar-style menu to pick which app opens a file
- **Search filter** — type to filter files in the current directory
- **Copy / Cut / Paste** — `Ctrl+C`, `Ctrl+X`, `Ctrl+V`
- **Multi-select** — `Ctrl+Click` or `Shift+Click`
- **Hidden files toggle** — `Ctrl+H`
- **Rename** — `F2` inline rename
- **Delete** — `Delete` sends to trash
- **Open terminal here** — `F4` opens your terminal in the current directory
- **One Dark theme** — matches SwordDeck desktop HUD
- **Recursive search** — search files inside subdirectories

---

## Installation

### Option 1: Install from .deb (Ubuntu / Debian)

```bash
# Download
wget https://github.com/BayazidHabibSiddikee/SwordFM/releases/download/v1.0.0/swordfm-1.0.0-amd64.deb

# Install
sudo dpkg -i swordfm-1.0.0-amd64.deb

# Fix missing dependencies if needed
sudo apt-get install -f

# Run
swordfm
```

### Option 2: Install from portable tarball (any Linux)

```bash
# Download
wget https://github.com/BayazidHabibSiddikee/SwordFM/releases/download/v1.0.0/swordfm-1.0.0-linux-x64.tar.gz

# Extract
tar xzf swordfm-1.0.0-linux-x64.tar.gz
cd SwordFM

# Run directly
./swordfm

# Or install system-wide
sudo cp swordfm /usr/local/bin/
swordfm
```

### Option 3: Build from source

#### Dependencies

**Arch Linux:**
```bash
sudo pacman -S qt6-base cmake gcc
```

**Ubuntu / Debian:**
```bash
sudo apt install qt6-base-dev cmake g++ build-essential
```

**Fedora:**
```bash
sudo dnf install qt6-qtbase-devel cmake gcc-c++
```

#### Build & Install

```bash
git clone https://github.com/BayazidHabibSiddikee/SwordFM.git
cd SwordFM

# Build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Install to ~/.local/bin (no sudo needed)
./install.sh

# Or install system-wide (sudo needed)
sudo cmake --install build --prefix /usr/local
```

---

## Make It Permanent

### Set as default file manager

**Option A: xdg-mime (recommended)**

```bash
# Set SwordFM as default for directories
xdg-mime default swordfm.desktop inode/directory

# Verify
xdg-mime query default inode/directory
# Should print: swordfm.desktop
```

**Option B: Manual config**

Edit `~/.config/mimeapps.list` and add:

```ini
[Default Applications]
inode/directory=swordfm.desktop
```

### Add to i3 autostart

Add to `~/.config/i3/config`:

```bash
exec --no-startup-id swordfm ~
```

### Add to i3 keybind

Add to `~/.config/i3/config`:

```bash
bindsym $mod+e exec swordfm ~
```

### Add to desktop environment

SwordFM installs a `.desktop` file. After install, it appears in:
- Application menus
- "Open with" dialogs
- Desktop environment file manager settings

To register manually:

```bash
mkdir -p ~/.local/share/applications
cp /usr/share/applications/swordfm.desktop ~/.local/share/applications/
update-desktop-database ~/.local/share/applications/
```

### Set as default in XFCE / GNOME / KDE

**XFCE:**
```bash
xfconf-query -c xfce4-settings-manager -p /default-applications/file-manager -s swordfm
```

**GNOME:** Settings → Default Applications → Video / Music / etc.

**KDE:** System Settings → Applications → Default Applications → File Manager

### Autostart on login (generic)

Create `~/.config/autostart/swordfm.desktop`:

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

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+L` | Focus path bar (type a path to jump) |
| `Alt+←` | Back |
| `Alt+→` | Forward |
| `Alt+↑` / `Backspace` | Go up one directory |
| `F5` | Refresh |
| `Ctrl+H` | Toggle hidden files |
| `F2` | Rename selected file |
| `Delete` | Delete (send to trash) |
| `Space` / `F3` | Toggle preview panel |
| `F4` | Open terminal here |
| `Ctrl+1` | Details view |
| `Ctrl+2` | Icon view |
| `Ctrl+C` | Copy |
| `Ctrl+X` | Cut |
| `Ctrl+V` | Paste |
| `Ctrl+A` | Select all |
| `Ctrl+N` | New folder |
| `Enter` | Open file / enter directory |
| `Ctrl+Q` | Quit |

---

## Architecture

```
src/
├── app/                 Entry point, main window, theme
│   ├── main.cpp         Application bootstrap, One Dark palette
│   ├── mainwindow.cpp   Main window shell, layout, shortcuts
│   └── theme.h          Color constants, stylesheet
│
├── model/               Data layer
│   ├── filemodel.cpp    Qt filesystem model wrapper
│   ├── filefilter.cpp   Filter proxy (hidden files, search)
│   ├── searchmodel.cpp  Recursive file search
│   └── filescanner.cpp  Background directory scanner
│
├── view/                File listing
│   └── fileview.cpp     Details + icon view pages
│
├── panel/               Chrome around the listing
│   ├── sidebar.cpp      Places / Devices / Bookmarks
│   ├── toolbar.cpp      Navigation bar, path bar, search
│   ├── statusbar.cpp    File count, selection size
│   └── previewpanel.cpp Live file preview
│
└── ops/                 File operations
    ├── fileops.cpp      Copy, cut, paste, delete, rename
    ├── archiveops.cpp   Extract archives (zip, tar, etc.)
    ├── convertops.cpp   Document conversion (via swordconv)
    ├── shareops.cpp     LAN file sharing (via swordshare)
    ├── contextmenu.cpp  Right-click menu
    ├── openwith.cpp     Open With dialog (XDG aware)
    └── termutil.cpp     Open terminal in current directory
```

---

## Related Projects

- **[SwordDeck](https://github.com/BayazidHabibSiddikee/CyberPaper)** — Animated desktop HUD with mission graph, system stats, and audio visualizer
- **swordconv** — Document conversion helper (PDF, DOCX, MD, TXT, HTML)
- **swordgraph** — Visualize folder structure as a graph
- **swordshare** — Password-protected LAN file sharing with QR code

---

## License

MIT
