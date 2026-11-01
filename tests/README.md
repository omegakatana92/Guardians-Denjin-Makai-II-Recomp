# Tests

The native build needs the user's own `grdians.zip`; no ROM is in this
repository. Set `GRDIANS_ROM` to its path before running these.

## ROM and translation

```bash
python tools/verify_rom.py "$GRDIANS_ROM"
python tools/recompile68k.py "$GRDIANS_ROM"
```

Expected: `13/13 files verified`; translation reports 7,638 instructions and no
unsupported instruction.

## Headless boot

```bash
build/cmake/bin/guardians_headless --rom "$GRDIANS_ROM" --frames 600 \
    --hashes build/boot.hashes
```

Expected (2026-10-31, GCC 13.2):

- completes 600 frames;
- no `missing translated code` line;
- `unmapped writes 0`;
- reaches the interrupt-driven loop at `00095C`;
- writes one hash line per frame.

The 1,200-frame run was also verified with no missing code and 1,129 unmapped
reads (open-bus reads past the RAM/ROM regions, not writes).

## Deterministic reference comparison

`tools/reference/mame_capture.lua` is a development-only MAME script that writes
PC samples, work-RAM dumps, video-register dumps and PNG snapshots. It is not
needed to build or run the port and is not part of the game.

With the reference dumps:

```bash
python tools/compare_ram.py build/native_300.bin build/reference/ram_300.bin
```

(If `compare_ram.py` is not present, the one-line check is to compare bytes with
Python.)

At frame 300, 65,234 of 65,536 work-RAM bytes matched MAME (99.54%). Differences
were timer/RNG counters and sprite-object state.

## Manual front end

```bash
build/cmake/bin/Guardians-Windows-x86_64.exe --rom "$GRDIANS_ROM"
```

The SDL2 window was confirmed to start and remain responsive for five seconds
with the local ROM. Video correctness and audio are **not** accepted yet.
