"""Compare two RAM dumps (e.g. native vs MAME) byte by byte."""
from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native", type=Path)
    parser.add_argument("reference", type=Path)
    args = parser.parse_args()
    a = args.native.read_bytes()
    b = args.reference.read_bytes()
    if len(a) != len(b):
        print(f"size mismatch: {len(a)} vs {len(b)}")
        return 2
    equal = sum(x == y for x, y in zip(a, b))
    differences = [i for i, (x, y) in enumerate(zip(a, b)) if x != y]
    print(f"{equal}/{len(a)} bytes identical ({100.0 * equal / len(a):.2f}%)")
    print(f"{len(differences)} differing bytes")
    for offset in differences[:32]:
        print(f"  {offset:04X}: native {a[offset]:02X}  reference {b[offset]:02X}")
    return 0 if not differences else 1


if __name__ == "__main__":
    raise SystemExit(main())
