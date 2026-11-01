"""Static translation of the Guardians 68000 program into C.

The 68000 text is decoded once, here, by tools/core/m68000/disassembler.py.
Each instruction becomes a small C function that calls a semantic helper
(runtime/src/m68k_ops.c); the runtime never fetches or decodes an instruction.

Generated C is ROM-derived and is written only under build/generated/, which is
git-ignored.  Nothing here is redistributable.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))
sys.path.insert(0, str(TOOLS / "core"))

import game_profile  # noqa: E402
import rom_set  # noqa: E402
from m68000 import disassembler as dis  # noqa: E402

# The core's Analysis module imports the game profile itself; keep its code
# range in step with ours before importing.
game_profile.CODE_START = 0x100
from m68000 import analysis as core_analysis  # noqa: E402

core_analysis.CODE_START = 0x100

CONDITIONS = {name: index for index, name in enumerate(dis.CONDITIONS)}
SIZES = {None: 2, "b": 1, "w": 2, "l": 4}
SHIFTS = {"asl": 0, "asr": 1, "lsl": 2, "lsr": 3, "rol": 4, "ror": 5,
          "roxl": 6, "roxr": 7}
BITOPS = {"btst": 0, "bchg": 1, "bclr": 2, "bset": 3}


def reg_number(name: str) -> int:
    kind = name[0]
    number = int(name[1:])
    return number + (8 if kind == "A" else 0)


def condition_number(name: str | None) -> int:
    if name is None:
        raise ValueError("conditional instruction without a condition")
    return CONDITIONS[name]


def operand_ea(op) -> str:
    mode = op.mode
    if mode == "dreg":
        return f"EA_DREG({reg_number(op.reg)})"
    if mode == "areg":
        return f"EA_AREG({reg_number(op.reg) & 7})"
    if mode == "indirect":
        return f"EA_IND({reg_number(op.reg) & 7})"
    if mode == "postinc":
        return f"EA_POST({reg_number(op.reg) & 7})"
    if mode == "predec":
        return f"EA_PRE({reg_number(op.reg) & 7})"
    if mode == "disp":
        return f"EA_DISP({reg_number(op.reg) & 7}, {op.disp})"
    if mode == "index":
        index = reg_number(op.index)
        long_index = 1 if op.index_size == "l" else 0
        return f"EA_INDEX({reg_number(op.reg) & 7}, {index}, {long_index}, {op.disp})"
    if mode in ("abs_short", "abs_long", "pc_disp"):
        return f"EA_ABS(0x{op.value & 0xFFFFFF:06X}u)"
    if mode == "pc_index":
        address = (op.base + op.disp) & 0xFFFFFF
        index = reg_number(op.index)
        long_index = 1 if op.index_size == "l" else 0
        return f"EA_ABS_INDEX(0x{address:06X}u, {index}, {long_index})"
    if mode in ("immediate", "quick"):
        return f"EA_IMM(0x{op.value & 0xFFFFFFFF:08X}u)"
    raise ValueError(f"unsupported effective address mode {mode}")


def sized(mnemonic: str, size: str | None) -> str:
    """A C identifier suffix for the helper, if the helper is size-specific."""
    return mnemonic


def emit_instruction(instruction, next_address: int) -> tuple[list[str], str]:
    """Return (statements, return-expression) for one instruction."""
    mnemonic = instruction.mnemonic
    size = SIZES[instruction.size]
    ops = instruction.operands
    statements: list[str] = []

    if mnemonic in ("nop", "reset"):
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "move":
        if len(ops) == 1:
            # MOVEM is decoded as movem; a single operand here is not expected.
            raise ValueError(f"move with one operand: {instruction.text}")
        src, dst = ops
        if dst.mode == "sr":
            if src.mode in ("immediate", "quick"):
                statements.append(f"m68k_set_sr(s, 0x{src.value & 0xFFFF:04X}u);")
            else:
                statements.append(f"m68k_move_to_sr(s, {operand_ea(src)}, {size});")
            return statements, f"0x{next_address:06X}u"
        if dst.mode == "ccr":
            if src.mode in ("immediate", "quick"):
                statements.append(f"m68k_set_ccr(s, 0x{src.value & 0xFF:02X}u);")
            else:
                statements.append(f"m68k_move_to_ccr(s, {operand_ea(src)}, {size});")
            return statements, f"0x{next_address:06X}u"
        if src.mode == "sr":
            statements.append(f"m68k_move_from_sr(s, {operand_ea(dst)}, {size});")
            return statements, f"0x{next_address:06X}u"
        if src.mode == "ccr":
            statements.append(f"m68k_move_from_ccr(s, {operand_ea(dst)}, {size});")
            return statements, f"0x{next_address:06X}u"
        statements.append(f"m68k_move(s, {operand_ea(src)}, {operand_ea(dst)}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "movea":
        statements.append(f"m68k_movea(s, {operand_ea(ops[0])}, {reg_number(ops[1].reg) & 7}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "moveq":
        statements.append(f"m68k_moveq(s, (int32_t)0x{ops[0].value & 0xFFFFFFFF:08X}u, {reg_number(ops[1].reg)});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "lea":
        statements.append(f"m68k_lea(s, {operand_ea(ops[0])}, {reg_number(ops[1].reg) & 7});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "pea":
        statements.append(f"m68k_pea(s, {operand_ea(ops[0])});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic in ("add", "sub"):
        if ops[1].mode == "areg":
            helper = "m68k_adda" if mnemonic == "add" else "m68k_suba"
            statements.append(f"{helper}(s, {operand_ea(ops[0])}, {reg_number(ops[1].reg) & 7}, {size});")
        else:
            helper = "m68k_add" if mnemonic == "add" else "m68k_sub"
            statements.append(f"{helper}(s, {operand_ea(ops[0])}, {operand_ea(ops[1])}, {size}, 0);")
        return statements, f"0x{next_address:06X}u"

    if mnemonic in ("adda", "suba"):
        helper = "m68k_adda" if mnemonic == "adda" else "m68k_suba"
        statements.append(f"{helper}(s, {operand_ea(ops[0])}, {reg_number(ops[1].reg) & 7}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic in ("addi", "subi", "andi", "ori", "eori"):
        value = ops[0].value & 0xFFFFFFFF
        dst = ops[1]
        if dst.mode == "ccr":
            helper = {"andi": "m68k_andi_ccr", "ori": "m68k_ori_ccr", "eori": "m68k_eori_ccr"}.get(mnemonic)
            if helper is None:
                raise ValueError(f"{mnemonic} to CCR is not valid")
            statements.append(f"{helper}(s, 0x{value:08X}u);")
            return statements, f"0x{next_address:06X}u"
        if dst.mode == "sr":
            if mnemonic == "andi":
                statements.append(f"m68k_set_sr(s, s->sr & 0x{value:08X}u);")
            elif mnemonic == "ori":
                statements.append(f"m68k_set_sr(s, s->sr | 0x{value:08X}u);")
            elif mnemonic == "eori":
                statements.append(f"m68k_set_sr(s, s->sr ^ 0x{value:08X}u);")
            else:
                raise ValueError(f"{mnemonic} to SR is not valid")
            return statements, f"0x{next_address:06X}u"
        statements.append(f"m68k_{mnemonic}(s, 0x{value:08X}u, {operand_ea(dst)}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic in ("addq", "subq"):
        helper = "m68k_addq" if mnemonic == "addq" else "m68k_subq"
        statements.append(f"{helper}(s, 0x{ops[0].value & 0xFFFFFFFF:08X}u, {operand_ea(ops[1])}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic in ("addx", "subx"):
        helper = "m68k_addx" if mnemonic == "addx" else "m68k_subx"
        statements.append(f"{helper}(s, {operand_ea(ops[0])}, {reg_number(ops[1].reg)}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "cmp":
        if ops[1].mode == "areg":
            statements.append(f"m68k_cmpa(s, {operand_ea(ops[0])}, {reg_number(ops[1].reg) & 7}, {size});")
        else:
            statements.append(f"m68k_cmp(s, {operand_ea(ops[0])}, {operand_ea(ops[1])}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "cmpa":
        statements.append(f"m68k_cmpa(s, {operand_ea(ops[0])}, {reg_number(ops[1].reg) & 7}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "cmpi":
        statements.append(f"m68k_cmpi(s, 0x{ops[0].value & 0xFFFFFFFF:08X}u, {operand_ea(ops[1])}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic in ("and", "or", "eor"):
        statements.append(f"m68k_{mnemonic}(s, {operand_ea(ops[0])}, {operand_ea(ops[1])}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic in ("clr", "tst", "neg", "not"):
        statements.append(f"m68k_{mnemonic}(s, {operand_ea(ops[0])}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "swap":
        statements.append(f"m68k_swap(s, {reg_number(ops[0].reg)});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "ext":
        statements.append(f"m68k_ext(s, {reg_number(ops[0].reg)}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic in ("mulu", "muls", "divu", "divs"):
        statements.append(f"m68k_{mnemonic}(s, {operand_ea(ops[0])}, {reg_number(ops[1].reg)});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic in SHIFTS:
        if ops[0].mode == "quick":
            count = f"0x{ops[0].value & 0xFFFFFFFF:08X}u"
        elif ops[0].mode == "dreg":
            count = f"(s->d[{reg_number(ops[0].reg)}] & 63u)"
        else:
            raise ValueError(f"unsupported shift count: {instruction.text}")
        statements.append(f"m68k_shift(s, {SHIFTS[mnemonic]}, {count}, {operand_ea(ops[1])}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic in BITOPS:
        bit = ops[0]
        if bit.mode in ("immediate", "quick"):
            bit_expr = f"0x{bit.value & 0xFFFFFFFF:08X}u"
        elif bit.mode == "dreg":
            bit_expr = f"s->d[{reg_number(bit.reg)}]"
        else:
            raise ValueError(f"unsupported bit source: {instruction.text}")
        statements.append(f"m68k_bitop(s, {BITOPS[mnemonic]}, {bit_expr}, {operand_ea(ops[1])});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "movem":
        if ops[0].mode == "reglist":
            mask = sum(1 << reg_number(name) for name in ops[0].registers)
            statements.append(f"m68k_movem_to_mem(s, 0x{mask:04X}u, {operand_ea(ops[1])}, {size});")
        else:
            mask = sum(1 << reg_number(name) for name in ops[1].registers)
            statements.append(f"m68k_movem_to_regs(s, 0x{mask:04X}u, {operand_ea(ops[0])}, {size});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "link":
        statements.append(f"m68k_link(s, {reg_number(ops[0].reg) & 7}, (int32_t)0x{ops[1].value & 0xFFFFFFFF:08X}u);")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "unlk":
        statements.append(f"m68k_unlk(s, {reg_number(ops[0].reg) & 7});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "exg":
        first, second = ops
        both_data = first.mode == "dreg" and second.mode == "dreg"
        statements.append(f"m68k_exg(s, {reg_number(first.reg)}, {reg_number(second.reg)}, {1 if both_data else 0});")
        return statements, f"0x{next_address:06X}u"

    if mnemonic == "bra":
        return statements, f"0x{instruction.target & 0xFFFFFF:06X}u"

    if mnemonic == "bsr":
        statements.append(f"m68k_push32(s, 0x{next_address:06X}u);")
        return statements, f"0x{instruction.target & 0xFFFFFF:06X}u"

    if mnemonic == "bcc":
        cond = condition_number(instruction.condition)
        if cond == 0:
            return statements, f"0x{instruction.target & 0xFFFFFF:06X}u"
        if cond == 1:
            return statements, f"0x{next_address:06X}u"
        return statements, (f"m68k_cond(s, {cond}) ? 0x{instruction.target & 0xFFFFFF:06X}u "
                            f": 0x{next_address:06X}u")

    if mnemonic == "dbcc":
        cond = condition_number(instruction.condition)
        return statements, (f"m68k_dbcc(s, {reg_number(ops[0].reg)}, {cond}, "
                            f"0x{instruction.target & 0xFFFFFF:06X}u) "
                            f"? 0x{instruction.target & 0xFFFFFF:06X}u : 0x{next_address:06X}u")

    if mnemonic == "jmp":
        if instruction.target is not None:
            return statements, f"0x{instruction.target & 0xFFFFFF:06X}u"
        return statements, f"m68k_ea_address(s, {operand_ea(ops[0])})"

    if mnemonic == "jsr":
        statements.append(f"m68k_push32(s, 0x{next_address:06X}u);")
        if instruction.target is not None:
            return statements, f"0x{instruction.target & 0xFFFFFF:06X}u"
        return statements, f"m68k_ea_address(s, {operand_ea(ops[0])})"

    if mnemonic == "rts":
        return statements, "m68k_pop32(s)"
    if mnemonic == "rte":
        statements.append("m68k_rte(s);")
        return statements, "s->pc"
    if mnemonic == "rtr":
        statements.append("m68k_rtr(s);")
        return statements, "s->pc"

    if mnemonic == "trap":
        vector = ops[0].value & 15
        statements.append(f"m68k_trap(s, {vector});")
        return statements, "s->pc"

    if mnemonic == "stop":
        statements.append(f"m68k_stop(s, 0x{ops[0].value & 0xFFFF:04X}u);")
        return statements, f"0x{next_address:06X}u"

    raise ValueError(f"unsupported instruction at {instruction.address:06X}: {instruction.text}")


def collect_analysis(image: bytes, seeds: list[int]):
    analysis = core_analysis.Analysis(image, {})
    reset_pc = rom_set.read_vector(image, 1)
    analysis.enter(reset_pc, kind="func")
    for vector in range(1, 0x49):
        target = rom_set.read_vector(image, vector) & 0xFFFFFF
        if 0x100 <= target < rom_set.PROGRAM_SIZE:
            analysis.enter(target, kind="func")
    for seed in seeds:
        if 0x100 <= seed < rom_set.PROGRAM_SIZE and seed % 2 == 0 and analysis.clean(seed):
            analysis.enter(seed, kind="func")
    analysis.run()
    return analysis


def load_seeds(path: Path) -> list[int]:
    seeds = []
    if not path.exists():
        return seeds
    for line in path.read_text(encoding="ascii").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) < 2 or parts[0] not in ("func", "code"):
            raise SystemExit(f"{path}: bad seed line: {line}")
        seeds.append(int(parts[1], 16) & 0xFFFFFF)
    return seeds


def generate(image: bytes, seeds: list[int], out_dir: Path, per_file: int = 900) -> dict:
    analysis = collect_analysis(image, seeds)
    out_dir.mkdir(parents=True, exist_ok=True)
    for old in out_dir.glob("program_*.c"):
        old.unlink()
    for old in out_dir.glob("program_*.h"):
        old.unlink()

    addresses = sorted(analysis.instructions)
    unsupported = []
    bodies: dict[int, tuple[str, str]] = {}
    for address in addresses:
        instruction = analysis.instructions[address]
        next_address = address + instruction.length
        try:
            statements, result = emit_instruction(instruction, next_address)
        except ValueError as error:
            unsupported.append(str(error))
            continue
        bodies[address] = (instruction.text, statements, result)
    if unsupported:
        raise SystemExit("unsupported instructions:\n  " + "\n  ".join(unsupported[:20]))

    chunks = []
    for start in range(0, len(addresses), per_file):
        chunk_addresses = addresses[start:start + per_file]
        lines = [
            '/* Generated by tools/recompile68k.py from the user-supplied ROM.',
            ' * ROM-derived code: never commit or redistribute this file. */',
            '#include "m68k/m68k.h"',
            '#include "grdians/bus.h"',
            '',
        ]
        for address in chunk_addresses:
            text, statements, result = bodies[address]
            lines.append(f"/* {address:06X}: {text} */")
            lines.append(f"uint32_t b_{address:06X}(m68k_state *s)")
            lines.append("{")
            for statement in statements:
                lines.append(f"    {statement}")
            lines.append(f"    return {result};")
            lines.append("}")
            lines.append("")
        chunk = out_dir / f"program_{start // per_file:02d}.c"
        chunk.write_text("\n".join(lines) + "\n", encoding="ascii")
        chunks.append(chunk)

    dispatch = [
        '/* Generated dispatch table for the translated program. */',
        '#include "m68k/m68k.h"',
        '#include <stdint.h>',
        '#include <stddef.h>',
        '',
    ]
    for address in addresses:
        dispatch.append(f"uint32_t b_{address:06X}(m68k_state *s);")
    dispatch += ['', 'typedef struct { uint32_t key; uint32_t (*fn)(m68k_state *); } m68k_entry;',
                 '', 'static const m68k_entry entries[] = {']
    for address in addresses:
        dispatch.append(f"    {{ 0x{address:06X}u, b_{address:06X} }},")
    dispatch += ['};', '', f'#define ENTRY_COUNT {len(addresses)}u',
                 '#define HASH_BITS 17u', '#define HASH_SIZE (1u << HASH_BITS)',
                 'static uint32_t keys[HASH_SIZE];',
                 'static uint16_t slots[HASH_SIZE];',
                 'static int ready;',
                 '',
                 'static uint32_t hash_pc(uint32_t pc) {',
                 '    pc &= 0xFFFFFEu;',
                 '    pc ^= pc >> 17; pc *= 0x9E3779B1u; pc ^= pc >> 13;',
                 '    return (pc >> 1) & (HASH_SIZE - 1u);',
                 '}',
                 '',
                 'void m68k_program_init(void) {',
                 '    if (ready) return;',
                 '    for (unsigned i = 0; i < ENTRY_COUNT; i++) {',
                 '        uint32_t slot = hash_pc(entries[i].key);',
                 '        while (keys[slot]) slot = (slot + 1u) & (HASH_SIZE - 1u);',
                 '        keys[slot] = entries[i].key;',
                 '        slots[slot] = (uint16_t)i;',
                 '    }',
                 '    ready = 1;',
                 '}',
                 '',
                 'uint32_t m68k_program_step(m68k_state *s, uint32_t pc) {',
                 '    uint32_t slot = hash_pc(pc);',
                 '    while (keys[slot]) {',
                 '        if (keys[slot] == pc) return entries[slots[slot]].fn(s);',
                 '        slot = (slot + 1u) & (HASH_SIZE - 1u);',
                 '    }',
                 '    return 0xFFFFFFFFu;',
                 '}',
                 '',
                 'uint32_t m68k_program_entry_count(void) { return ENTRY_COUNT; }',
                 'uint32_t m68k_program_missing(uint32_t pc) { (void)pc; return 0; }',
                 '']
    (out_dir / "program_dispatch.c").write_text("\n".join(dispatch) + "\n", encoding="ascii")

    header = [
        '#ifndef GRDIANS_PROGRAM_H',
        '#define GRDIANS_PROGRAM_H',
        '#include <stdint.h>',
        '#include "m68k/m68k.h"',
        'void m68k_program_init(void);',
        'uint32_t m68k_program_step(m68k_state *s, uint32_t pc);',
        'uint32_t m68k_program_entry_count(void);',
        '#endif',
        '',
    ]
    (out_dir / "program.h").write_text("\n".join(header), encoding="ascii")

    return {
        "instructions": len(addresses),
        "functions": len(analysis.functions),
        "labels": len(analysis.labels),
        "chunks": len(chunks),
        "bytes": sum(len(c.read_bytes()) for c in chunks),
        "range": (min(addresses), max(addresses)),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path, nargs="?",
                        default=Path(r"C:\Users\omega\Downloads\grdians.zip"))
    parser.add_argument("--seeds", type=Path, default=ROOT / "tools" / "seeds.txt")
    parser.add_argument("--out", type=Path, default=ROOT / "build" / "generated")
    args = parser.parse_args()

    image = rom_set.build_program(args.rom)
    seeds = load_seeds(args.seeds)
    stats = generate(image, seeds, args.out)
    print(f"program image: {len(image):#x} bytes, reset PC {rom_set.read_vector(image, 1):06X}")
    print(f"instructions: {stats['instructions']}, functions: {stats['functions']}, "
          f"labels: {stats['labels']}")
    print(f"range: {stats['range'][0]:06X}-{stats['range'][1]:06X}; "
          f"{stats['chunks']} C files, {stats['bytes']} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
