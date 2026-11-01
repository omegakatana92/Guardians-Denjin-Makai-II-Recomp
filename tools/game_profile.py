"""Game profile for the shared arcade-decomp core (tools/core).

The core imports this module before its CPUs and analysis; it supplies the ROM
access the analysis needs and the code range of the program image.  Seta 2 is
board-specific and is NOT part of the upstream core; runtime/ holds our model.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
import rom_set  # noqa: E402

rom = rom_set
GAME = "Guardians: Denjin Makai II"
ROM_SET_TITLE = "grdians"
CPU = "TMP68301"
BINARY = "grdians"
REFERENCE = "grdians_ref"

# The 68000 vector table occupies 0x000-0x0FF.  Code starts after it.
CODE_START = 0x200
PROGRAM_SIZE = rom_set.PROGRAM_SIZE
