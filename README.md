# Guardians: Denjin Makai II — Native Port Research

A genuine native recompilation of the 1995 Seta 2 arcade game **Guardians
(Denjin Makai II)** for Windows and Linux/Steam Deck. The 68000 program is
translated ahead of time into C by this repository's translator and compiled for
the host. The runtime models the Seta 2 board; no emulator core is linked into the
game and no instruction fetch/decode remains in the shipped binary.

> **You must own the arcade board or a legally obtained ROM.** This repository
> contains no ROM data, no extracted graphics or audio, and no translated
> ROM-derived C. The build tools read `grdians.zip` from your machine and write
> generated output under the ignored `build/` directory only.

## Status (verified 2026-10-31)

- **Milestone 0 — ROM verification and static analysis: complete.**
  `tools/verify_rom.py` checks all 13 files against MAME 0.289's `grdians`
  metadata; the archive matches exactly. The static analysis decodes the reset
  path, exception and autovector handlers, the TMP68301 initialisation, the
  ROM/RAM self-check, the sound-driver code and the interrupt-driven main loop
  at `00095C`. `tools/seeds.txt` records dynamically discovered entry points.
- **Milestone 1 — static translation, board model, headless boot: complete.**
  `tools/recompile68k.py` translates 7,638 instructions into 443 analysed
  functions with no unsupported instruction. The headless runner reaches the main
  loop, services VBLANK interrupts, and runs 1,200 frames without missing
  translated code. At frame 300 the native work RAM matches MAME's reference
  dump byte-for-byte for 99.54% of the 64 KiB (the remaining differences are
  timer/RNG counters and sprite-display object state).
- **Milestone 2 — video: in progress, not correct yet.**
  The X1-020/DX-101 sprite renderer, palette, sprite-list reformatting and a PNG
  writer are implemented. The native self-check text is visible, but its screen
  position and the sprite-object state do not yet match MAME; the attract/title
  sequence and gameplay rendering are **not** validated. This is the current
  blocker.
- **Windows SDL2 front end: builds and starts.** It was launched successfully
  with the local ROM. Audio is not implemented yet (X1-010 registers are stored,
  but no sample mixer).
- **Linux/Steam Deck: build files and packaging scripts exist, not built or
  tested in this environment.**

The project is **not playable** and no release is published.

## Requirements

- Python 3.11+
- CMake 3.20+ and a C11 compiler (tested with GCC 13.2 / MinGW-w64)
- Ninja (recommended)
- SDL2 development package for the SDL2 front end
- The exact MAME `grdians` ROM set (13 files, CRC32/SHA-1 checked)

The upstream `arcade-decomp` core is a pinned submodule at `tools/core/` (branch
`main`, commit recorded by the parent repository).

## Quick start (Windows)

```powershell
git clone --recurse-submodules https://github.com/omegakatana92/Guardians-Denjin-Makai-II-Recomp.git
cd Guardians-Denjin-Makai-II-Recomp
python tools\verify_rom.py C:\Users\you\Downloads\grdians.zip
.\scripts\build-local.ps1 -Rom C:\Users\you\Downloads\grdians.zip
build\cmake\bin\guardians_headless.exe --rom C:\Users\you\Downloads\grdians.zip --frames 600 --hashes build\boot.hashes
build\cmake\bin\Guardians-Windows-x86_64.exe --rom C:\Users\you\Downloads\grdians.zip
```

On Linux replace `\` with `/` and use `scripts/build-local.sh`.

## Controls (SDL2 front end)

| Key | Action |
| --- | --- |
| Arrows / WASD | Move |
| Z / J | Button 1 |
| X / K | Button 2 |
| C / L | Button 3 |
| Enter | Start 1P |
| 5 | Coin 1P |
| 1 | Service |
| F11 | Toggle fullscreen |
| F12 | Save `guardians-shot.png` |
| Esc | Quit |

Gamepad: D-pad and A/B/X map to the same controls; Start is 1P Start, Back is
Coin 1P.

## How it works

1. `tools/verify_rom.py` checks MAME's exact `grdians` file set.
2. `tools/recompile68k.py` decodes the program with the shared 68000 frontend,
   follows direct and seeded control flow, and emits one C function per
   instruction under `build/generated/`.
3. `runtime/src/m68k_ops.c` implements the 68000 semantics those functions call
   (flags, effective addresses, exceptions, MOVEM, shifts, BCD-less subset used
   by this program).
4. `runtime/src/board.c` implements the Seta 2 bus: 2 MiB program ROM, 64 KiB
   work RAM, the `0x304000` tile RAM, TMP68301 registers, inputs, watchdog,
   X1-010 register window, X1-020 sprite/palette/video registers and sample
   banks. The ROM archive is read at run time with miniz; no ROM bytes are
   compiled into the executable.
5. `ports/headless` and `ports/sdl2` run translated blocks, deliver VBLANK /
   raster interrupts, and present the 304×232 visible area.

The generated C is ROM-derived and must never be committed or redistributed.

## Tests

```powershell
python tools\verify_rom.py C:\Users\you\Downloads\grdians.zip
python tools\recompile68k.py C:\Users\you\Downloads\grdians.zip
build\cmake\bin\guardians_headless.exe --rom C:\Users\you\Downloads\grdians.zip --frames 600 --hashes build\boot.hashes
```

`tools/iterate_seeds.py` is a development helper that runs the native program,
adds any address it could not translate to `tools/seeds.txt`, regenerates and
rebuilds. It stores addresses only, never ROM bytes.

## Repository layout

```
tools/          ROM loader, verifier, translator, seed iteration
tools/core/     arcade-decomp submodule (not modified)
runtime/        Seta 2 board + 68000 semantic helpers + PNG writer
ports/headless/ deterministic frame runner and hash log
ports/sdl2/     window, keyboard/gamepad, fullscreen
docs/           ROM analysis, hardware plan, build and Steam Deck notes
packaging/      AppImage/desktop helpers (Linux, untested here)
tests/          test scripts and notes
```

## Licensing

The repository's original code is under the GPL-2.0 in `LICENSE`. The pinned
`arcade-decomp` core is BSD-3-Clause (see `tools/core/LICENSE`). miniz is MIT
(`runtime/third_party/miniz/LICENSE`). SDL2 is zlib-licensed. Third-party
notices are collected under `LICENSES/`; they are not replaced by the GPL.

No game assets, ROMs, or translated ROM-derived C are licensed or distributed by
this repository.
