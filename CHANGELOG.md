# Changelog

## Unreleased — 2026-10-31

### Added

- Exact MAME 0.289 `grdians` ROM verification (13 files, CRC32 + SHA-1).
- `tools/rom_set.py`: 2 MiB program image, 32 MiB MAME-compatible graphics
  region, 1 MiB X1-010 sample image.
- `tools/recompile68k.py`: 68000 static translation to C using the shared
  `m68000` decoder, with seeds for dynamically reached handlers and script
  dispatch tables.
- `runtime/src/m68k_ops.c`: 68000 semantics, exception frames, flags, EA modes,
  MOVEM, shifts, bit operations, MUL/DIV, ADDX/SUBX.
- `runtime/src/board.c`: Seta 2 bus, RAM, TMP68301 registers, inputs, watchdog,
  X1-010 window, X1-020 sprite/palette/video registers, ROM loader.
- X1-020/DX-101 sprite rendering (normal + floating tilemap), xRGB-555 palette
  and PNG snapshots.
- Headless runner with per-frame hashes and RAM/sprite/palette/graphics dumps.
- SDL2 Windows front end with keyboard, gamepad, fullscreen and F12 screenshot.
- `tools/iterate_seeds.py`: native-run-driven discovery of untranslated entry
  points; appends addresses only.
- Build, CI, AppImage and licensing scaffolding.

### Verified

- `verify_rom.py`: 13/13 files match MAME.
- Translator: 7,638 instructions, 443 functions, no unsupported instruction.
- Headless: 1,200 frames, no missing translated code, 0 unmapped writes.
- Native work RAM at frame 300: 99.54% byte-identical to MAME's reference
  (differences are timer/RNG/object state).
- Graphics region and palette: byte-identical to MAME's built regions.
- Windows SDL2 executable builds and starts with the local ROM.

### Known limitations

- Video is not yet correct: self-check text is rendered at the wrong screen
  position and sprite object state diverges from MAME.
- Audio is not mixed.
- Linux/Steam Deck binaries and the AppImage are not built or tested.
- Raster IRQ line timing and TMP68301 timers are approximate.

No release artifacts are published.
