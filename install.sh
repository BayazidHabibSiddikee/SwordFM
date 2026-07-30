#!/bin/bash
# install.sh — Build and install SwordFM
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="${PREFIX:-$HOME/.local}"

echo "=== Building SwordFM ==="
cmake -B "$HERE/build" -S "$HERE" -DCMAKE_BUILD_TYPE=Release
cmake --build "$HERE/build" -j"$(nproc)"
echo "  -> $HERE/build/swordfm OK"

echo ""
echo "=== Installing to $PREFIX/bin ==="
install -Dm755 "$HERE/build/swordfm" "$PREFIX/bin/swordfm"

mkdir -p "$HOME/.local/share/applications"
cat > "$HOME/.local/share/applications/swordfm.desktop" << 'EOF'
[Desktop Entry]
Type=Application
Name=SwordFM
Comment=Thunar-like Qt6 file manager
Exec=swordfm %f
Icon=system-file-manager
Terminal=false
Categories=Utility;Core;FileManager;
MimeType=inode/directory;
EOF

echo "  -> Installed swordfm to $PREFIX/bin/"
echo "  -> Desktop entry: ~/.local/share/applications/swordfm.desktop"
echo ""
echo "Done! Run: swordfm"
