# Building on Windows 10/11 (x86-64)

Tested on Windows 11 with GCC 13.2 (MinGW-w64 via Strawberry Perl's toolchain),
CMake 4.3, Ninja, Python 3.14 and SDL2 from MSYS2 (`C:\msys64\mingw64`).

## 1. Prerequisites

- Python 3.11+ (`python --version`)
- CMake 3.20+ and Ninja
- A 64-bit C11 compiler. This repository was built with:
  `C:\Strawberry\c\bin\gcc.exe`
- SDL2 development files. Either install the Visual Studio SDL2 VC package or
  use MSYS2's `mingw-w64-x86_64-SDL2`.
- Your own `grdians.zip`.

The build does not download anything.

## 2. Verify the ROM

```powershell
python tools\verify_rom.py C:\Users\you\Downloads\grdians.zip
```

Expected: `13/13 files verified` and exit code 0.

## 3. Configure and build

```powershell
cmake -S . -B build\cmake -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_COMPILER=gcc `
  -DGRDIANS_ROM=C:\Users\you\Downloads\grdians.zip
cmake --build build\cmake
```

If SDL2 is not found, CMake still builds `guardians_headless`; the SDL2 target is
`guardians_sdl2` with output
`build\cmake\bin\Guardians-Windows-x86_64.exe`.

The translator runs at configure time and writes ROM-derived C under
`build/generated/`. That directory is git-ignored. Re-run CMake after changing
`tools/seeds.txt` or the translator.

## 4. Run

```powershell
build\cmake\bin\guardians_headless.exe `
  --rom C:\Users\you\Downloads\grdians.zip `
  --frames 600 --hashes build\boot.hashes

build\cmake\bin\Guardians-Windows-x86_64.exe `
  --rom C:\Users\you\Downloads\grdians.zip
```

The SDL2 executable needs `SDL2.dll` beside it. With MSYS2:

```powershell
Copy-Item C:\msys64\mingw64\bin\SDL2.dll build\cmake\bin\
```

## 5. Expected results

- The headless runner prints `program: reset PC 000180` and completes the frame
  count with `unmapped writes 0`.
- At 300 frames the ROM/RAM self-check has run; the native work-RAM hash is
  stable and close to MAME's reference but not identical (timer/object state).
- The SDL2 window opens at 304×232 × scale. Video is **not yet correct**; see
  `docs/HARDWARE_PLAN.md`.

## Troubleshooting

- `recompile68k.py failed` at configure: check the ROM hashes and the Python
  path; the translator refuses an unsupported instruction rather than falling
  back to an interpreter.
- `undefined reference to SDL_*`: the SDL2 development library is missing or is
  a different architecture. Use a 64-bit SDL2.
- The executable stops with `missing translated code at XXXXXX`: this is the
  deliberate proof-of-concept failure mode. Add the address to
  `tools/seeds.txt` only after confirming it is code, then regenerate. Do not
  add a runtime interpreter.
