# Seta 2 hardware plan — Guardians

This document records the board model implemented in `runtime/` and the parts
that remain open. The reference is MAME 0.289 (`seta2.cpp`,
`x1_020_dx_101.cpp`, `x1_010.cpp`, `tmp68301.cpp`); MAME is not linked into the
game.

## Implemented

- **TMP68301-compatible 68000 core semantics.** The translated C calls
  `runtime/src/m68k_ops.c`; there is no runtime instruction fetch/decode.
  Exception frames, SR/IPL masking, TRAP, RTE/RTR/RTS, STOP, DBcc, MOVEM,
  shifts, bit operations, MUL/DIV, ADDX/SUBX and the game's effective-address
  modes are implemented.
- **Bus and memory.** 2 MiB program ROM; 64 KiB work RAM at 0x200000; 48 KiB
  tile RAM at 0x304000; DSW1/DSW2 at 0x600000; P1/P2/SYSTEM at 0x700000;
  watchdog read; coin counter byte at 0x800001; X1-010 word window at 0xB00000;
  sprite RAM 0xC00000-0xC3FFFF; palette 0xC40000; extra palette 0xC50000; video
  registers 0xC60000; sample banks 0xE00010; TMP68301 internal registers
  0xFFFC00-0xFFFFFF.
- **Interrupts.** VBLANK is delivered as TMP68301 external interrupt 0, level 4,
  vector 0x40. Raster is external interrupt 1, vector 0x41. The 68000 exception
  frame is pushed through the RAM stack.
- **Inputs.** Active-low P1/P2/SYSTEM reads with keyboard and SDL gamepad mapping
  in the front end.
- **Video.** X1-020/DX-101 sprite-list reformatting, 8bpp tile decode, palette
  (xRGB_555), normal and floating-tilemap sprite paths, global offsets and
  zoom fields, PNG output.
- **ROM handling.** The user's ZIP is validated against MAME's names, sizes,
  CRC32 and SHA-1, then read at run time with miniz. No ROM bytes are compiled
  into the executable.

## Open / incomplete

- **Video accuracy.** The native self-check text is visible, but its position and
  the sprite-object state differ from MAME. The attract/title sequence, character
  select and gameplay are not validated. The likely cause is a remaining CPU
  semantic/timing difference in the object/display-list builder around
  0x00A892, not the tile/palette decode (the 32 MiB gfx region and 64 KiB palette
  match MAME byte-for-byte).
- **Raster timing.** Raster IRQ is accepted but not scheduled at the exact line;
  only VBLANK is delivered each frame.
- **TMP68301 timers.** Registers and the timer-0 programming are stored; timer
  interrupts and the periodic timer are not modelled. Guardians masks timer IRQs
  during normal play, so this has not blocked boot.
- **X1-010 audio.** Register writes and sample banking are captured; no voices,
  envelopes or mixing.
- **Inputs.** Test/service and coin lockout bookkeeping are minimal.
- **Frame timing.** The headless/SDL loop uses an instruction budget
  (35,000 instructions per frame) as a cycle proxy. A per-instruction cycle
  model is needed for exact MAME frame/state comparison.

## Acceptance criteria

- ROM identification against MAME hashes: done.
- Every implemented range has a MAME-cited semantic: in progress.
- Same deterministic scenario matches MAME state and presentation: state is
  close (99.5% of work RAM at frame 300); presentation is not yet accepted.
- Windows executable builds and starts: done. Linux/AppImage: not yet built or
  tested in this environment.

## Constraints

Never distribute ROMs or copied assets. Do not call a MAME-wrapped executable a
native recompilation. Generated C is ROM-derived and stays under the ignored
`build/` tree.
