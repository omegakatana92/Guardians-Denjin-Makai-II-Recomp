#!/usr/bin/env bash
# Build the Linux AppImage.  Requires appimagetool or linuxdeploy on PATH.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

BUILD_DIR=${BUILD_DIR:-build/cmake}
APPDIR=${APPDIR:-build/AppDir}
DIST=${DIST:-dist}
VERSION=${VERSION:-0.1.0}

if [[ ! -x "$BUILD_DIR/bin/Guardians" ]]; then
  echo "error: build the Linux binary first (scripts/build-local.sh)" >&2
  exit 1
fi

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/256x256/apps" "$DIST"
cp "$BUILD_DIR/bin/Guardians" "$APPDIR/usr/bin/guardians"
cp packaging/appimage/guardians.desktop "$APPDIR/guardians.desktop"
cp packaging/appimage/guardians.png "$APPDIR/guardians.png"
cp packaging/appimage/AppRun "$APPDIR/AppRun"
chmod +x "$APPDIR/AppRun" "$APPDIR/usr/bin/guardians"
cp packaging/appimage/guardians.png "$APPDIR/usr/share/icons/hicolor/256x256/apps/guardians.png"
cp packaging/appimage/guardians.desktop "$APPDIR/usr/share/applications/guardians.desktop"

# Bundle non-system shared libraries (SDL2 and its dependencies).
if command -v linuxdeploy >/dev/null 2>&1; then
  linuxdeploy --appdir "$APPDIR" --executable "$APPDIR/usr/bin/guardians" --output appimage
  mv Guardians*.AppImage "$DIST/Guardians-SteamDeck-x86_64.AppImage"
elif command -v appimagetool >/dev/null 2>&1; then
  appimagetool "$APPDIR" "$DIST/Guardians-SteamDeck-x86_64.AppImage"
else
  echo "error: appimagetool or linuxdeploy is required" >&2
  exit 1
fi

echo "wrote $DIST/Guardians-SteamDeck-x86_64.AppImage (ROM not included)"
