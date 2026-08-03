#!/bin/bash
# install.sh — Build and install SwordFM + helpers (swordshare, swordgraph, swordconv)
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="${PREFIX:-$HOME/.local}"
BINDIR="$PREFIX/bin"
DESKTOP="$HOME/.local/share/applications/swordfm.desktop"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}  ✔ $*${NC}"; }
info() { echo -e "${YELLOW}  → $*${NC}"; }
die()  { echo -e "${RED}  ✖ $*${NC}" >&2; exit 1; }

echo ""
echo "══════════════════════════════════════════"
echo "   SwordFM  —  build & install"
echo "══════════════════════════════════════════"

# ── 1. Build dependencies ───────────────────────────────────────────────────
echo ""
info "Checking build dependencies..."

MISSING=()
command -v cmake &>/dev/null || MISSING+=(cmake)
command -v g++   &>/dev/null || MISSING+=(g++)
pkg-config --exists Qt6Core 2>/dev/null || {
    if   command -v apt-get &>/dev/null; then MISSING+=(qt6-base-dev)
    elif command -v dnf     &>/dev/null; then MISSING+=(qt6-qtbase-devel)
    elif command -v pacman  &>/dev/null; then MISSING+=(qt6-base)
    fi
}

if [ ${#MISSING[@]} -gt 0 ]; then
    info "Installing missing packages: ${MISSING[*]}"
    if   command -v apt-get &>/dev/null; then sudo apt-get install -y "${MISSING[@]}"
    elif command -v dnf     &>/dev/null; then sudo dnf install -y "${MISSING[@]}"
    elif command -v pacman  &>/dev/null; then sudo pacman -S --needed --noconfirm "${MISSING[@]}"
    else die "Please install manually: ${MISSING[*]}"
    fi
fi
ok "Build dependencies satisfied (Qt $(pkg-config --modversion Qt6Core 2>/dev/null || echo '?'))"

# ── 2. Python check (for helpers) ──────────────────────────────────────────
echo ""
info "Checking Python 3..."
command -v python3 &>/dev/null || die "python3 not found. Install it first."
ok "Python $(python3 --version)"

# ── 3. Build SwordFM ───────────────────────────────────────────────────────
echo ""
info "Configuring..."
cmake -B "$HERE/build" -S "$HERE" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -Wno-dev -DCMAKE_RULE_MESSAGES=OFF -DCMAKE_INSTALL_MESSAGE=NEVER \
    > /dev/null

info "Compiling with $(nproc) threads..."
cmake --build "$HERE/build" -j"$(nproc)" 2>&1 | tail -3
ok "Build complete"

# ── 4. Install binary ──────────────────────────────────────────────────────
echo ""
info "Installing SwordFM..."
mkdir -p "$BINDIR"
install -Dm755 "$HERE/build/swordfm" "$BINDIR/swordfm"
ok "swordfm  → $BINDIR/swordfm"

# ── 5. Install helpers ─────────────────────────────────────────────────────
TOOLS_DIR="$HERE/tools"
if [ -d "$TOOLS_DIR" ]; then
    for helper in swordshare swordgraph swordconv; do
        if [ -f "$TOOLS_DIR/$helper" ]; then
            install -Dm755 "$TOOLS_DIR/$helper" "$BINDIR/$helper"
            ok "$helper  → $BINDIR/$helper"
        else
            echo -e "  - $helper not found in tools/, skipping"
        fi
    done
else
    echo -e "  - tools/ directory not found, skipping helpers"
fi

# ── 6. Desktop entry ───────────────────────────────────────────────────────
mkdir -p "$HOME/.local/share/applications"
cat > "$DESKTOP" << 'EOF'
[Desktop Entry]
Type=Application
Name=SwordFM
GenericName=File Manager
Comment=Thunar-like Qt6 file manager
Exec=swordfm %f
Icon=system-file-manager
Terminal=false
StartupNotify=true
Categories=Utility;Core;FileManager;
MimeType=inode/directory;
Keywords=files;folders;file manager;
EOF
ok "Desktop entry → $DESKTOP"
command -v update-desktop-database &>/dev/null && \
    update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true

# ── 7. PATH check ──────────────────────────────────────────────────────────
echo ""
if ! echo "$PATH" | tr ':' '\n' | grep -qx "$BINDIR"; then
    SHELL_RC=""
    case "$SHELL" in
        */zsh)  SHELL_RC="$HOME/.zshrc"  ;;
        */bash) SHELL_RC="$HOME/.bashrc" ;;
    esac
    if [ -n "$SHELL_RC" ] && ! grep -q "$BINDIR" "$SHELL_RC" 2>/dev/null; then
        printf '\n# Added by SwordFM install.sh\nexport PATH="$HOME/.local/bin:$PATH"\n' \
            >> "$SHELL_RC"
        info "Added $BINDIR to PATH in $SHELL_RC — run: source $SHELL_RC"
    else
        info "Add to your shell config: export PATH=\"$BINDIR:\$PATH\""
    fi
else
    ok "$BINDIR is in PATH"
fi

# ── Done ───────────────────────────────────────────────────────────────────
echo ""
echo "══════════════════════════════════════════"
echo -e "${GREEN}  SwordFM installed successfully!${NC}"
echo "══════════════════════════════════════════"
echo "  Run:        swordfm"
echo "  Uninstall:  ./uninstall.sh"
echo ""
