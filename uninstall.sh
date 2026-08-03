#!/bin/bash
# uninstall.sh — Remove SwordFM and helpers from the system
set -euo pipefail

PREFIX="${PREFIX:-$HOME/.local}"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}  ✔ $*${NC}"; }
info() { echo -e "${YELLOW}  → $*${NC}"; }
skip() { echo -e "  - $* (not found, skipping)"; }

remove_file() {
    local f="$1"
    if [ -f "$f" ]; then
        if [[ "$f" == /usr/* ]]; then
            sudo rm -f "$f"
        else
            rm -f "$f"
        fi
        ok "Removed: $f"
        return 0
    fi
    return 1
}

echo ""
echo "══════════════════════════════════════════"
echo "   SwordFM  —  uninstall"
echo "══════════════════════════════════════════"
echo ""

# ── Binaries ───────────────────────────────────────────────────────────────
for exe in swordfm swordshare swordgraph swordconv; do
    found=0
    for loc in \
        "$PREFIX/bin/$exe" \
        "$HOME/.local/bin/$exe" \
        "/usr/local/bin/$exe" \
        "/usr/bin/$exe"
    do
        remove_file "$loc" && found=1 && break
    done
    [ $found -eq 0 ] && skip "$exe"
done

# ── Desktop entry ──────────────────────────────────────────────────────────
DESKTOP_FOUND=0
for loc in \
    "$HOME/.local/share/applications/swordfm.desktop" \
    "/usr/local/share/applications/swordfm.desktop" \
    "/usr/share/applications/swordfm.desktop"
do
    remove_file "$loc" && DESKTOP_FOUND=1
done
[ $DESKTOP_FOUND -eq 0 ] && skip "desktop entry"
command -v update-desktop-database &>/dev/null && \
    update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true

# ── Build directory ────────────────────────────────────────────────────────
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -d "$HERE/build" ]; then
    rm -rf "$HERE/build"
    ok "Removed: $HERE/build"
else
    skip "build directory"
fi

# ── xdg-mime default reset ─────────────────────────────────────────────────
if command -v xdg-mime &>/dev/null; then
    CURRENT=$(xdg-mime query default inode/directory 2>/dev/null || true)
    if [ "$CURRENT" = "swordfm.desktop" ]; then
        MIMEAPPS="$HOME/.config/mimeapps.list"
        [ -f "$MIMEAPPS" ] && sed -i '/^inode\/directory=swordfm\.desktop/d' "$MIMEAPPS"
        ok "Reset default file manager in mimeapps.list"
    fi
fi

# ── Done ───────────────────────────────────────────────────────────────────
echo ""
echo "══════════════════════════════════════════"
echo -e "${GREEN}  SwordFM uninstalled.${NC}"
echo "══════════════════════════════════════════"
echo ""
