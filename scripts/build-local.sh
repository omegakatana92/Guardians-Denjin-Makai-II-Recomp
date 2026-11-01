#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 /path/to/grdians.zip [build-dir]" >&2
  exit 2
fi

ROM=$1
BUILD_DIR=${2:-build/cmake}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

python3 tools/verify_rom.py "$ROM"
cmake -S . -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DGRDIANS_ROM="$ROM"
cmake --build "$BUILD_DIR"

"$BUILD_DIR/bin/guardians_headless" --rom "$ROM" --frames 600 --hashes build/boot.hashes
echo "Build and headless smoke test passed."
