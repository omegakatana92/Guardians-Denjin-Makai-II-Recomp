# ROM analysis — Guardians (grdians)

Date: 2026-10-31
Local archive: `C:\Users\omega\Downloads\grdians.zip` (14,199,857 bytes)
Reference: MAME 0.289, `src/mame/seta/seta2.cpp`, `ROM_START( grdians )`.

## Verification

`python tools/verify_rom.py` checks name, size, CRC32 and SHA-1 for every file.
The archive matches MAME's P-FG01-1 set exactly: **13/13 files**.

| File | Size | CRC32 | SHA-1 |
| --- | ---: | --- | --- |
| u2.bin | 0x080000 | 36adc6f2 | 544e87f88179fe1342e7a06a8948ac1828e85108 |
| u3.bin | 0x080000 | 2704f416 | 9081a12cbb9927d36e1c50b52aa2c6003810ee42 |
| u4.bin | 0x080000 | bb52447b | 61433f683210ab2bc2cf1cc4b5b7a39cc5b6493d |
| u5.bin | 0x080000 | 9c164a3b | 6d688c7af9e7e8e8d54b2e4dfbf41f59c79242eb |
| u16.bin | 0x400000 | 6a65f265 | 6cad11f718f8bbcff464d41eb4717460769237ed |
| u15.bin | 0x400000 | 01672dcd | f61f60e3343cc5b6ccee391ee529966a141566db |
| u18.bin | 0x400000 | 967babf4 | 42a6311576417c44aeaceb8ba6bb3cd7794e4882 |
| u17.bin | 0x400000 | 0fad0629 | 1bdc8e7c5e39e83d327f14a672ec81b049112da6 |
| u20.bin | 0x400000 | a7226ab7 | 408580dd35c568ffef1ebbd87359e3ec1f867020 |
| u19.bin | 0x400000 | c0c998a0 | 498fb1877527ed37412537f06a2c39ff0c60f146 |
| u22.bin | 0x400000 | 6239997a | 87b6d6f30f152f625f82fd858c1290176c7e156e |
| u21.bin | 0x400000 | 6f95e466 | 28482fad16a3ac9302f152d81552e6f84a44f3e4 |
| u32.bin | 0x100000 | cf0f3017 | 8376d3a674f71aec72f52c72758fbc53d9feb1a1 |

The same archive is accepted by `mame.exe -verifyroms grdians` (`romset grdians
is good`).

## Executable layout

The program image is 2 MiB, built from 16-bit interleaved byte-wide ROMs:

| Image range | Source | Bus range |
| --- | --- | --- |
| 0x000000-0x0FFFFF | u2 (even), u3 (odd) | 0x000000-0x1FFFFF |
| 0x100000-0x1FFFFF | u4 (even), u5 (odd) | 0x000000-0x1FFFFF (high half) |

Reset vector: SP `0x0020FFF0`, PC `0x000180` (big-endian words at image offsets
0 and 4). The vector table occupies 0x0000-0x00FF and the first real code starts
at 0x0180; the analysis therefore starts at 0x0100.

The reset path writes the TMP68301 address-decoder, serial, parallel, timer and
interrupt-controller registers, clears sprite RAM at 0xC00000, checks the
program checksum (0x00026A-0x00028E), tests 64 KiB at 0x200000 and 48 KiB at
0x304000, then initialises video, sound and the display list. It enables
interrupts at 0x000958 and parks in the counter loop at 0x00095C:

```
00095C  addq.w #1,($104,A6)
000960  bra     $95C
```

All game work runs from interrupts. The program sets `IVNR` to 0x40; external
interrupt 0 (VBLANK) therefore vectors through 0x0100 to `0x00B7B2`, and external
interrupt 1 (raster) through 0x0104 to `0x00B2DA`. `0x00B7B2` reads the inputs,
runs the display-list/object code and writes the video registers.

There is no CPU-side encryption or program bank switching on this set. The
sound sample window at 0xB00000 is an X1-010 register file, and sample banking is
eight byte writes at 0xE00010-0xE0001F.

## Video region

MAME's 32 MiB `video` region is built with `ROM_LOAD64_WORD`, i.e. each source
word occupies one 16-bit lane of an 8-byte group:

| Region range | Source | Loading |
| --- | --- | --- |
| 0x0000000-0x0FFFFFF | u16/u15/u18/u17 lanes 0/2/4/6 | one 4 MiB load |
| 0x1000000-0x17FFFFF | u20/u19/u22/u21 second halves | `ROM_CONTINUE` |
| 0x1800000-0x1FFFFFF | u20/u19/u22/u21 first halves | `ROM_LOAD64_WORD` |

`tools/rom_set.py` and the runtime loader were both checked against MAME's own
`video` memory region (`manager.machine.memory.regions[":video"]`): all
33,554,432 bytes match.

The graphics are 8×8, 8bpp tiles, 64 bytes per tile (512 bits; eight bitplanes
of 64 bits each). Tiles are selected by `code % 524288`; the code is 19 bits.

## Sound region

`u32.bin` is the 1 MiB X1-010 sample ROM. The X1-010 has 16 voices, 0x2000 bytes
of register/envelope/wave RAM and a 16-bit word interface at 0xB00000. The
current runtime stores register writes and sample-bank selections but does not
yet mix audio.

## Test ROM

Only this exact MAME `grdians` set is supported. `grdiansa` has identical program
ROMs but a different clock source and PCB; it is not tested. Bootleg and
regional sets are rejected by the hash check.
