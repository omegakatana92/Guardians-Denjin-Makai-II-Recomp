"""Guardians: Denjin Makai II ROM set definitions and loaders.

Authoritative metadata comes from MAME's `grdians` driver (src/mame/seta/seta2.cpp,
ROM_START(grdians)).  Nothing here writes ROM data into the repository; callers
pass the user's own archive and build images in memory or under build/.
"""
from __future__ import annotations

import hashlib
import struct
import zipfile
from dataclasses import dataclass
from pathlib import Path

PROGRAM_SIZE = 0x200000
GRAPHICS_SIZE = 0x2000000
SOUND_SIZE = 0x100000

# The program ROM occupies one contiguous 2 MiB area in the 68000's 24-bit space.
PROGRAM_AREAS = ((0x000000, PROGRAM_SIZE),)


@dataclass(frozen=True)
class RomFile:
    name: str
    size: int
    crc32: int
    sha1: str


# ROM_START( grdians ) // P-FG01-1 PCB, MAME 0.289
ROM_FILES = (
    RomFile("u2.bin", 0x80000, 0x36ADC6F2, "544e87f88179fe1342e7a06a8948ac1828e85108"),
    RomFile("u3.bin", 0x80000, 0x2704F416, "9081a12cbb9927d36e1c50b52aa2c6003810ee42"),
    RomFile("u4.bin", 0x80000, 0xBB52447B, "61433f683210ab2bc2cf1cc4b5b7a39cc5b6493d"),
    RomFile("u5.bin", 0x80000, 0x9C164A3B, "6d688c7af9e7e8e8d54b2e4dfbf41f59c79242eb"),
    RomFile("u16.bin", 0x400000, 0x6A65F265, "6cad11f718f8bbcff464d41eb4717460769237ed"),
    RomFile("u15.bin", 0x400000, 0x01672DCD, "f61f60e3343cc5b6ccee391ee529966a141566db"),
    RomFile("u18.bin", 0x400000, 0x967BABF4, "42a6311576417c44aeaceb8ba6bb3cd7794e4882"),
    RomFile("u17.bin", 0x400000, 0x0FAD0629, "1bdc8e7c5e39e83d327f14a672ec81b049112da6"),
    RomFile("u20.bin", 0x400000, 0xA7226AB7, "408580dd35c568ffef1ebbd87359e3ec1f867020"),
    RomFile("u19.bin", 0x400000, 0xC0C998A0, "498fb1877527ed37412537f06a2c39ff0c60f146"),
    RomFile("u22.bin", 0x400000, 0x6239997A, "87b6d6f30f152f625f82fd858c1290176c7e156e"),
    RomFile("u21.bin", 0x400000, 0x6F95E466, "28482fad16a3ac9302f152d81552e6f84a44f3e4"),
    RomFile("u32.bin", 0x100000, 0xCF0F3017, "8376d3a674f71aec72f52c72758fbc53d9feb1a1"),
)

PROGRAM_FILES = ROM_FILES[:4]
VIDEO_FILES = ROM_FILES[4:12]
SOUND_FILES = ROM_FILES[12:]


def sha1(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


def crc32(data: bytes) -> int:
    return zipfile.crc32(data) & 0xFFFFFFFF


@dataclass
class Check:
    name: str
    ok: bool
    detail: str


def check_archive(path: Path) -> list[Check]:
    """Check every required file's name, size, CRC32 and SHA-1."""
    checks: list[Check] = []
    try:
        archive = zipfile.ZipFile(path)
    except (OSError, zipfile.BadZipFile) as error:
        return [Check(path.name, False, f"cannot open archive: {error}")]
    names = set(archive.namelist())
    for required in ROM_FILES:
        if required.name not in names:
            checks.append(Check(required.name, False, "missing"))
            continue
        data = archive.read(required.name)
        if len(data) != required.size:
            checks.append(Check(required.name, False,
                                f"size {len(data):#x}, expected {required.size:#x}"))
        elif crc32(data) != required.crc32:
            checks.append(Check(required.name, False,
                                f"CRC32 {crc32(data):08x}, expected {required.crc32:08x}"))
        elif sha1(data) != required.sha1:
            checks.append(Check(required.name, False, f"SHA-1 {sha1(data)}, expected {required.sha1}"))
        else:
            checks.append(Check(required.name, True, f"{len(data):#x} CRC32 {required.crc32:08x}"))
    extra = sorted(names - {item.name for item in ROM_FILES})
    if extra:
        checks.append(Check("(extra files)", True, ", ".join(extra)))
    return checks


def _read(archive: zipfile.ZipFile, name: str) -> bytes:
    data = archive.read(name)
    entry = next(item for item in ROM_FILES if item.name == name)
    if len(data) != entry.size or crc32(data) != entry.crc32:
        raise ValueError(f"{name}: not the verified grdians file")
    return data


def build_program(path: Path) -> bytes:
    """Build the 2 MiB program image: u2/u3 at 0, u4/u5 at 0x100000 (16-bit interleaved)."""
    with zipfile.ZipFile(path) as archive:
        image = bytearray(PROGRAM_SIZE)
        for name, offset in (("u2.bin", 0x000000), ("u3.bin", 0x000001),
                             ("u4.bin", 0x100000), ("u5.bin", 0x100001)):
            data = _read(archive, name)
            image[offset:offset + len(data) * 2:2] = data
    return bytes(image)


def build_graphics(path: Path) -> bytes:
    """Build MAME's 32 MiB `video` region from ROM_LOAD64_WORD/ROM_CONTINUE entries.

    Each 16-bit word of a file goes to one byte lane of a 64-bit group.  MAME's
    ROM_LOAD64_WORD places the first word at lane 0 and advances by two bytes;
    the four chips therefore fill lanes 0, 2, 4 and 6 (big-endian words).
    The u19-u22 pair is loaded once at 0x1800000 and continued at 0x1000000.
    """
    with zipfile.ZipFile(path) as archive:
        region = bytearray(GRAPHICS_SIZE)
        for name, offset in (("u16.bin", 0x0000000), ("u15.bin", 0x0000002),
                             ("u18.bin", 0x0000004), ("u17.bin", 0x0000006),
                             ("u20.bin", 0x1800000), ("u19.bin", 0x1800002),
                             ("u22.bin", 0x1800004), ("u21.bin", 0x1800006)):
            data = _read(archive, name)
            if name in ("u20.bin", "u19.bin", "u22.bin", "u21.bin"):
                # 2 MiB at the first offset, then the second 2 MiB at 0x1000000.
                first, second = data[:0x200000], data[0x200000:]
                chunks = ((first, offset), (second, offset - 0x800000))
            else:
                chunks = ((data, offset),)
            for chunk, base in chunks:
                for index in range(0, len(chunk), 2):
                    destination = base + (index // 2) * 8
                    region[destination] = chunk[index]
                    region[destination + 1] = chunk[index + 1]
    return bytes(region)


def build_sound(path: Path) -> bytes:
    with zipfile.ZipFile(path) as archive:
        return _read(archive, "u32.bin")


def canonical_address(address: int) -> int | None:
    """The program image offset for a 68000 bus address, or None outside the ROM."""
    address &= 0xFFFFFF
    return address if address < PROGRAM_SIZE else None


def address_in_program(address: int) -> bool:
    return canonical_address(address) is not None


def load_set(path: Path) -> Path:
    """The core's ROM-set shim: the archive itself is enough for our loaders."""
    return Path(path)


def read_vector(image: bytes, vector: int) -> int:
    return struct.unpack_from(">I", image, vector * 4)[0]
