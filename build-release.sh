#!/bin/bash
# build-release.sh — Build SwordFM release artifacts (.deb and .tar.gz)
# Run from the repo root. Requires: cmake, g++, qt6-base-dev, dpkg-deb
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="1.0.0"
ARCH="amd64"
PKG="swordfm-${VERSION}"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}  ✔ $*${NC}"; }
info() { echo -e "${YELLOW}  → $*${NC}"; }
die()  { echo -e "${RED}  ✖ $*${NC}" >&2; exit 1; }

echo ""
echo "══════════════════════════════════════════"
echo "   SwordFM $VERSION  —  build release"
echo "══════════════════════════════════════════"

# ── 1. Check tools ─────────────────────────────────────────────────────────
for cmd in cmake g++ dpkg-deb; do
    command -v "$cmd" &>/dev/null || die "Missing required tool: $cmd"
done
pkg-config --exists Qt6Core 2>/dev/null || die "Qt6 dev headers not found. Install qt6-base-dev"
ok "Tools ready (Qt $(pkg-config --modversion Qt6Core))"

# ── 2. Build binary ────────────────────────────────────────────────────────
echo ""
info "Building swordfm..."
cmake -B "$HERE/build" -S "$HERE" \
    -DCMAKE_BUILD_TYPE=Release \
    -Wno-dev -DCMAKE_RULE_MESSAGES=OFF \
    > /dev/null
cmake --build "$HERE/build" -j"$(nproc)" 2>&1 | tail -3
ok "Binary built: $HERE/build/swordfm"

# ── 3. Build .deb ──────────────────────────────────────────────────────────
echo ""
info "Assembling .deb package..."

STAGING="$HERE/dist/deb-staging"
rm -rf "$STAGING"

# directory layout that dpkg-deb expects
mkdir -p "$STAGING/DEBIAN"
mkdir -p "$STAGING/usr/bin"
mkdir -p "$STAGING/usr/share/applications"
mkdir -p "$STAGING/usr/share/doc/swordfm"

# binary
install -Dm755 "$HERE/build/swordfm"        "$STAGING/usr/bin/swordfm"

# helpers
install -Dm755 "$HERE/tools/swordshare"     "$STAGING/usr/bin/swordshare"
install -Dm755 "$HERE/tools/swordgraph"     "$STAGING/usr/bin/swordgraph"
install -Dm755 "$HERE/tools/swordconv"      "$STAGING/usr/bin/swordconv"

# desktop entry
cat > "$STAGING/usr/share/applications/swordfm.desktop" << 'EOF'
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

# copyright
cat > "$STAGING/usr/share/doc/swordfm/copyright" << EOF
SwordFM $VERSION
MIT License — https://github.com/BayazidHabibSiddikee/SwordFM
EOF

# DEBIAN/control — compute installed size
INSTALLED_KB=$(du -sk "$STAGING/usr" | awk '{print $1}')
cat > "$STAGING/DEBIAN/control" << EOF
Package: swordfm
Version: $VERSION
Section: utils
Priority: optional
Architecture: $ARCH
Installed-Size: $INSTALLED_KB
Depends: libqt6widgets6 | libqt6widgets6t64, libqt6core6 | libqt6core6t64,
 python3, python3-pip,
 poppler-utils,
 tar, gzip, xz-utils, bzip2, zstd,
 zip, unzip, p7zip-full,
 graphviz
Maintainer: BayazidHabibSiddikee <your@email.com>
Description: SwordFM — Thunar-like Qt6 file manager
 A fast, keyboard-driven file manager built in C++20 with Qt6.
 Includes swordshare (LAN sharing), swordgraph (folder graph),
 and swordconv (document conversion).
 .
 The postinst script automatically installs all Python dependencies
 (pymupdf, mammoth, python-docx, pdf2docx, beautifulsoup4, markdown, qrcode).
EOF

# DEBIAN/postinst and prerm
install -Dm755 "$HERE/debian/postinst" "$STAGING/DEBIAN/postinst"
install -Dm755 "$HERE/debian/prerm"    "$STAGING/DEBIAN/prerm"

# build the .deb
mkdir -p "$HERE/dist"
DEB="$HERE/dist/${PKG}-${ARCH}.deb"
dpkg-deb --root-owner-group --build "$STAGING" "$DEB"
ok ".deb → $DEB"

# verify it
info "Verifying .deb..."
dpkg-deb -I "$DEB" | grep -E "Package|Version|Depends|Installed-Size"
ok ".deb is valid"

# ── 4. Build tarball ───────────────────────────────────────────────────────
echo ""
info "Building portable tarball..."

TAR_DIR="$HERE/dist/tar-staging/SwordFM"
rm -rf "$HERE/dist/tar-staging"
mkdir -p "$TAR_DIR"

install -Dm755 "$HERE/build/swordfm"    "$TAR_DIR/swordfm"
install -Dm755 "$HERE/tools/swordshare" "$TAR_DIR/swordshare"
install -Dm755 "$HERE/tools/swordgraph" "$TAR_DIR/swordgraph"
install -Dm755 "$HERE/tools/swordconv"  "$TAR_DIR/swordconv"
cp "$HERE/install.sh"                    "$TAR_DIR/install.sh"
cp "$HERE/uninstall.sh"                  "$TAR_DIR/uninstall.sh"
chmod +x "$TAR_DIR/install.sh" "$TAR_DIR/uninstall.sh"

# minimal README inside the tarball
cat > "$TAR_DIR/README.txt" << EOF
SwordFM $VERSION
================

Quick install (builds from source, no Qt version mismatch):
  cd SwordFM && ./install.sh

Or run directly:
  ./swordfm

Uninstall:
  ./uninstall.sh

Full docs: https://github.com/BayazidHabibSiddikee/SwordFM
EOF

TAR="$HERE/dist/${PKG}-linux-x64.tar.gz"
tar -czf "$TAR" -C "$HERE/dist/tar-staging" SwordFM
ok "tarball → $TAR"

# ── 5. Cleanup staging ─────────────────────────────────────────────────────
rm -rf "$STAGING" "$HERE/dist/tar-staging"

# ── Done ───────────────────────────────────────────────────────────────────
echo ""
echo "══════════════════════════════════════════"
echo -e "${GREEN}  Release artifacts ready:${NC}"
echo "══════════════════════════════════════════"
echo "  $DEB"
echo "  $TAR"
echo ""
echo "Upload both files to your GitHub release."
echo ""
