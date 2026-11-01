# Guardians Seta 2 hardware research plan

This is a planning document, not an implementation or an assertion of playability.

## Investigation areas
- Verify program ROM layout and endianness from the MAME driver and local archive.
- Identify the TMP68301/68000 memory map, interrupt sources, timers and peripheral register behavior.
- Model graphics command/data paths, palette, sprites and rendering order.
- Model sound chip registers, sample addressing, playback and frame synchronization.
- Establish inputs, dipswitches, credit handling and two-player controls.
- Capture deterministic reference traces and per-frame state/audio/video checksums using MAME.

## Acceptance criteria
- ROM identification is checked against known hashes.
- Every implemented memory range has reference-grounded semantics.
- The same deterministic scenario matches MAME state and presentation.
- Native output is confirmed on Windows before promising Linux/AppImage support.

## Constraints
Never distribute game ROMs or copied assets. Do not call a MAME-wrapped executable a native recompilation.
