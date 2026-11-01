"""Iterative static-analysis seeding.

Runs the translated headless program, and whenever it reaches an address that
the static analysis did not decode, appends that address to tools/seeds.txt,
regenerates and rebuilds.  Addresses only; no ROM data is stored.
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = Path(r"C:\Users\omega\Downloads\grdians.zip")
HEADLESS = ROOT / "build" / "cmake" / "bin" / "guardians_headless.exe"
SEEDS = ROOT / "tools" / "seeds.txt"


def main() -> int:
    frames = int(sys.argv[1]) if len(sys.argv) > 1 else 200
    iterations = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    for iteration in range(iterations):
        run = subprocess.run([str(HEADLESS), "--rom", str(ROM), "--frames", str(frames),
                              "--budget", "35000"], capture_output=True, text=True)
        output = run.stdout + run.stderr
        match = re.search(r"missing translated code at ([0-9A-F]{6})", output)
        if not match:
            print(f"iteration {iteration}: no missing code")
            print("\n".join(output.splitlines()[:12]))
            return 0
        address = int(match.group(1), 16)
        print(f"iteration {iteration}: seed {address:06X}")
        existing = SEEDS.read_text(encoding="ascii")
        line = f"func {address:06X} discovered by iterative native execution"
        if line not in existing:
            with SEEDS.open("a", encoding="ascii") as out:
                out.write(line + "\n")
        subprocess.run([sys.executable, str(ROOT / "tools" / "recompile68k.py"), str(ROM)],
                       check=True, capture_output=True)
        subprocess.run(["cmake", "-S", str(ROOT), "-B", str(ROOT / "build" / "cmake"),
                        "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
                        "-DCMAKE_C_COMPILER=gcc", f"-DGRDIANS_ROM={ROM}"],
                       check=True, capture_output=True)
        subprocess.run(["cmake", "--build", str(ROOT / "build" / "cmake")], check=True,
                       capture_output=True)
    print("iteration limit reached")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
