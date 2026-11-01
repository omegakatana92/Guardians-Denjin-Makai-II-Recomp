# Guardians: Denjin Makai II — Native Port Research

Experimental research project exploring a native recompilation of the 1995 Seta 2 arcade game **Guardians (Denjin Makai II)** for Windows, Linux, and Steam Deck.

**Status: research / not playable.** No compiled game or working hardware runtime is provided yet.

## Upstream toolchain

Candidate decompilation core: https://github.com/cheviman/arcade-decomp

The upstream project has a Motorola 68000 frontend, but does **not** provide a ready-to-use Seta 2 implementation. A CPU frontend alone is insufficient: the TMP68301 peripherals, memory map, graphics, sound, inputs, interrupts and frame timing still need a board runtime.

## Local game data

For initial development on Windows, use your own legally obtained ROM archive at:

`C:\Users\omega\Downloads\grdians.zip`

Keep it outside this repository. Do not commit ROMs, graphics dumps, samples, BIOS or other copyrighted game data.

Before decoding or modifying the game, compare the archive's ROM names, sizes and hashes with the appropriate MAME driver for Guardians. Regional sets and revisions may differ.

## Milestones

1. Verify ROM manifest and map the executable, graphics and audio regions.
2. Trace Guardians' Seta 2 hardware behavior against MAME and document missing interfaces.
3. Prototype a TMP68301-compatible 68000 analysis pipeline and board runtime.
4. Reach reproducible frame/state comparison against MAME.
5. Build and validate a native Windows executable, then Linux and Steam Deck AppImage.

A native recompilation must execute translated original program logic; simply bundling an emulator does not meet this project's stated goal.

## Development

Clone with `git clone https://github.com/omegakatana92/Guardians-Denjin-Makai-II-Recomp.git`. The upstream decompilation tools may later be incorporated as a pinned Git submodule after confirming integration details. No ROMs are required to clone this research repository.

## Licensing

See LICENSE for this repository's current terms. Third-party source retains its own license and attribution; adding the BSD-3-Clause arcade-decomp code requires preserving its applicable notices. No game's assets are licensed by this repository.
