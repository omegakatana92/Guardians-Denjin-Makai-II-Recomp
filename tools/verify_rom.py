"""Verify the player's grdians.zip against MAME 0.289's metadata."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import rom_set


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", nargs="?", type=Path,
                        default=Path("C:/Users/omega/Downloads/grdians.zip"))
    args = parser.parse_args()
    checks = rom_set.check_archive(args.rom)
    failed = 0
    for check in checks:
        print(f"{'OK  ' if check.ok else 'FAIL'} {check.name:<12} {check.detail}")
        failed += not check.ok
    required_ok = sum(1 for check in checks if check.ok and check.name != "(extra files)")
    print(f"\n{required_ok}/{len(rom_set.ROM_FILES)} files verified")
    if failed:
        print("ROM set does not match MAME's grdians set.", file=sys.stderr)
        return 1
    print("ROM set matches MAME 0.289 grdians exactly.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
