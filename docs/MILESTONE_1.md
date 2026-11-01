# Milestone 1 — static translation, Seta 2 board model, headless boot

Date: 2026-10-31
Host: Windows 11, GCC 13.2 (MinGW-w64), CMake 4.3, Ninja, Python 3.14
ROM: local `grdians.zip`, verified against MAME 0.289 (13/13 files)

## Commands run

```powershell
python tools\verify_rom.py C:\Users\omega\Downloads\grdians.zip
python tools\recompile68k.py C:\Users\omega\Downloads\grdians.zip
cmake -S . -B build\cmake -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_COMPILER=gcc -DGRDIANS_ROM=C:\Users\omega\Downloads\grdians.zip
cmake --build build\cmake
build\cmake\bin\guardians_headless.exe --rom C:\Users\omega\Downloads\grdians.zip `
  --frames 1200 --budget 35000 --hashes build\boot.hashes
build\cmake\bin\Guardians-Windows-x86_64.exe --rom C:\Users\omega\Downloads\grdians.zip
```

## Results

- ROM verification: 13/13, exact MAME `grdians` set.
- Translation: 7,638 instructions in 443 analysed functions, 9 generated C
  files, **no unsupported instruction**.
- Headless: 1,200 frames, 42,000,000 translated instructions, **no missing
  translated code**, **0 unmapped writes**, 1,129 open-bus reads.
- Boot path reached: reset 0x000180 → TMP68301 setup → checksum → RAM tests →
  video/sound init → interrupt enable at 0x000958 → main loop at 0x00095C.
- VBLANK at vector 0x40 (`0x00B7B2`) and raster at 0x41 (`0x00B2DA`) are
  delivered through the 68000 exception frame.
- At frame 300 the native work RAM matched MAME's reference dump for 65,234 of
  65,536 bytes (99.54%). The graphics region and palette match MAME's built
  memory regions byte-for-byte.
- SDL2 Windows executable builds and starts; it stayed alive for the five-second
  launch test.

## Known failures / open work

- The X1-020 sprite renderer shows the self-check text but at the wrong screen
  position and with divergent sprite-object state. Title/attract and gameplay
  are not validated.
- X1-010 audio is register-only; no sound.
- Raster IRQ is not line-scheduled; TMP68301 timers are not modelled.
- Linux/AppImage not built or tested in this environment.

## Artifacts (not committed)

- `build/generated/program_*.c` — ROM-derived, ignored.
- `build/boot.hashes` — per-frame hash log, ignored.
- `build/frame_*.png` — native snapshots, ignored for release.

No release was created.
