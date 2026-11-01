/* Seta 2 board implementation: bus, RAM, TMP68301 registers, inputs and ROM. */
#include "grdians/board.h"
#include "grdians/bus.h"
#include "miniz.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct rom_entry {
    const char *name;
    unsigned size;
    unsigned crc;
} rom_entry;

static const rom_entry rom_files[] = {
    { "u2.bin",  0x80000, 0x36ADC6F2 }, { "u3.bin",  0x80000, 0x2704F416 },
    { "u4.bin",  0x80000, 0xBB52447B }, { "u5.bin",  0x80000, 0x9C164A3B },
    { "u16.bin", 0x400000, 0x6A65F265 }, { "u15.bin", 0x400000, 0x01672DCD },
    { "u18.bin", 0x400000, 0x967BABF4 }, { "u17.bin", 0x400000, 0x0FAD0629 },
    { "u20.bin", 0x400000, 0xA7226AB7 }, { "u19.bin", 0x400000, 0xC0C998A0 },
    { "u22.bin", 0x400000, 0x6239997A }, { "u21.bin", 0x400000, 0x6F95E466 },
    { "u32.bin", 0x100000, 0xCF0F3017 },
};

static gr_board *g_board;
static void build_sprite_list(gr_board *board);

gr_bus_stats gr_bus_statistics;
int gr_bus_strict = 1;

static void bad_read(uint32_t address)
{
    gr_bus_statistics.unmapped_reads++;
    gr_bus_statistics.last_bad_read = address & 0xFFFFFF;
    if (gr_bus_strict && gr_bus_statistics.unmapped_reads <= 16)
        fprintf(stderr, "unmapped read  %06X at PC %06X\n", address & 0xFFFFFF,
                g_board ? g_board->cpu.pc : 0);
}

static void bad_write(uint32_t address)
{
    gr_bus_statistics.unmapped_writes++;
    gr_bus_statistics.last_bad_write = address & 0xFFFFFF;
    if (gr_bus_strict && gr_bus_statistics.unmapped_writes <= 16)
        fprintf(stderr, "unmapped write %06X at PC %06X\n", address & 0xFFFFFF,
                g_board ? g_board->cpu.pc : 0);
}

static inline uint16_t be16(const uint8_t *p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

static inline void put_be16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value >> 8);
    p[1] = (uint8_t)value;
}

uint8_t gr_bus_read8(uint32_t address)
{
    gr_bus_statistics.reads++;
    address &= 0xFFFFFF;
    gr_board *b = g_board;
    if (!b) return 0xFF;

    if (address < 0x200000) return b->program[address];
    if (address < 0x210000) return b->ram[address - 0x200000];
    if (address >= 0x304000 && address < 0x310000) return b->tile_ram[address - 0x304000];
    if (address == 0x600000 || address == 0x600001) {
        uint16_t v = 0xFFFF; /* SW1: normal difficulty, 2 lives, Guardians title */
        return address & 1 ? (uint8_t)v : (uint8_t)(v >> 8);
    }
    if (address == 0x600002 || address == 0x600003) {
        uint16_t v = 0xFFFF; /* SW2: 1 coin/1 credit, both slots */
        return address & 1 ? (uint8_t)v : (uint8_t)(v >> 8);
    }
    if (address == 0x700000 || address == 0x700001) {
        uint16_t v = (uint16_t)(0xFF00u | (~b->inputs & 0x00FFu));
        return address & 1 ? (uint8_t)v : (uint8_t)(v >> 8);
    }
    if (address == 0x700002 || address == 0x700003) {
        uint16_t v = (uint16_t)(0xFF00u | (~(b->inputs >> 8) & 0x00FFu));
        return address & 1 ? (uint8_t)v : (uint8_t)(v >> 8);
    }
    if (address == 0x700004 || address == 0x700005) {
        uint16_t v = 0xFFFF;
        if (b->inputs & GR_INPUT_COIN1) v &= ~0x0001u;
        if (b->inputs & GR_INPUT_COIN2) v &= ~0x0002u;
        if (b->inputs & GR_INPUT_SERVICE) v &= ~0x0004u;
        if (b->inputs & GR_INPUT_TEST) v &= ~0x0008u;
        return address & 1 ? (uint8_t)v : (uint8_t)(v >> 8);
    }
    if (address == 0x70000C || address == 0x70000D) return 0x00; /* watchdog */
    if (address >= 0xB00000 && address < 0xB04000) {
        uint32_t i = (address - 0xB00000) >> 1;
        uint16_t v = (uint16_t)((b->x1_hi[i] << 8) | b->x1_regs[i]);
        return address & 1 ? (uint8_t)v : (uint8_t)(v >> 8);
    }
    if (address >= 0xC00000 && address < 0xC40000) return b->spriteram[address - 0xC00000];
    if (address >= 0xC40000 && address < 0xC50000) {
        uint16_t v = b->palette[(address - 0xC40000) >> 1];
        return address & 1 ? (uint8_t)v : (uint8_t)(v >> 8);
    }
    if (address >= 0xC50000 && address < 0xC60000) {
        uint16_t v = b->extra_palette[(address - 0xC50000) >> 1];
        return address & 1 ? (uint8_t)v : (uint8_t)(v >> 8);
    }
    if (address >= 0xC60000 && address < 0xC60040) {
        uint16_t v = b->vregs[(address - 0xC60000) >> 1];
        return address & 1 ? (uint8_t)v : (uint8_t)(v >> 8);
    }
    if (address >= 0xFFFC00 && address < 0x1000000) return b->tmp[address - 0xFFFC00];
    bad_read(address);
    return 0xFF;
}

uint16_t gr_bus_read16(uint32_t address)
{
    gr_bus_statistics.reads++;
    address &= 0xFFFFFF;
    gr_board *b = g_board;
    if (!b) return 0xFFFF;
    if (address & 1) {
        gr_bus_statistics.odd_word_accesses++;
        if (gr_bus_strict) fprintf(stderr, "odd word read %06X\n", address);
    }
    if (address < 0x1FFFFF) return be16(&b->program[address]);
    if (address >= 0x200000 && address < 0x210000) {
        uint32_t o = address - 0x200000;
        return (uint16_t)((b->ram[o] << 8) | b->ram[o + 1]);
    }
    if (address >= 0x304000 && address < 0x310000) {
        uint32_t o = address - 0x304000;
        return (uint16_t)((b->tile_ram[o] << 8) | b->tile_ram[o + 1]);
    }
    if (address <= 0x700005 && address >= 0x600000) {
        uint8_t hi = gr_bus_read8(address);
        uint8_t lo = gr_bus_read8(address + 1);
        return (uint16_t)((hi << 8) | lo);
    }
    if (address >= 0xB00000 && address < 0xB04000) {
        uint32_t i = (address - 0xB00000) >> 1;
        return (uint16_t)((b->x1_hi[i] << 8) | b->x1_regs[i]);
    }
    if (address >= 0xC00000 && address < 0xC40000) {
        uint32_t o = address - 0xC00000;
        return (uint16_t)((b->spriteram[o] << 8) | b->spriteram[o + 1]);
    }
    if (address >= 0xC40000 && address < 0xC50000)
        return b->palette[(address - 0xC40000) >> 1];
    if (address >= 0xC50000 && address < 0xC60000)
        return b->extra_palette[(address - 0xC50000) >> 1];
    if (address >= 0xC60000 && address < 0xC60040)
        return b->vregs[(address - 0xC60000) >> 1];
    if (address >= 0xFFFC00 && address < 0x1000000) {
        uint32_t o = address - 0xFFFC00;
        return (uint16_t)((b->tmp[o] << 8) | b->tmp[o + 1]);
    }
    bad_read(address);
    return 0xFFFF;
}

uint32_t gr_bus_read32(uint32_t address)
{
    return ((uint32_t)gr_bus_read16(address) << 16) | gr_bus_read16(address + 2);
}

void gr_bus_write8(uint32_t address, uint8_t value)
{
    gr_bus_statistics.writes++;
    address &= 0xFFFFFF;
    gr_board *b = g_board;
    if (!b) return;
    if (address >= 0x200000 && address < 0x210000) { b->ram[address - 0x200000] = value; return; }
    if (address >= 0x304000 && address < 0x310000) { b->tile_ram[address - 0x304000] = value; return; }
    if (address >= 0xC00000 && address < 0xC40000) { b->spriteram[address - 0xC00000] = value; return; }
    if (address >= 0xC40000 && address < 0xC50000) {
        uint32_t i = (address - 0xC40000) >> 1;
        uint16_t v = b->palette[i];
        if (address & 1) v = (uint16_t)((v & 0xFF00) | value); else v = (uint16_t)((v & 0x00FF) | (value << 8));
        b->palette[i] = v;
        return;
    }
    if (address >= 0xC50000 && address < 0xC60000) {
        uint32_t i = (address - 0xC50000) >> 1;
        uint16_t v = b->extra_palette[i];
        if (address & 1) v = (uint16_t)((v & 0xFF00) | value); else v = (uint16_t)((v & 0x00FF) | (value << 8));
        b->extra_palette[i] = v;
        return;
    }
    if (address >= 0xB00000 && address < 0xB04000) {
        uint32_t i = (address - 0xB00000) >> 1;
        if (address & 1) {
            b->x1_regs[i] = value;
            if ((i % 8) == 0 && (value & 1)) { /* key-on reset */ }
        } else b->x1_hi[i] = value;
        return;
    }
    if (address >= 0xFFFC00 && address < 0x1000000) { b->tmp[address - 0xFFFC00] = value; return; }
    if (address == 0x800001) {
        b->coin_counter[0] = value & 1;
        b->coin_counter[1] = (value >> 1) & 1;
        return;
    }
    if (address >= 0xE00010 && address < 0xE00020) {
        b->x1_bank[(address - 0xE00010) & 7] = value;
        return;
    }
    if (address >= 0xE00000 && address < 0xE00010) return;
    if (address == 0x70000C || address == 0x70000D) return;
    bad_write(address);
}

void gr_bus_write16(uint32_t address, uint16_t value)
{
    gr_bus_statistics.writes++;
    address &= 0xFFFFFF;
    gr_board *b = g_board;
    if (!b) return;
    if (address & 1) {
        gr_bus_statistics.odd_word_accesses++;
        if (gr_bus_strict) fprintf(stderr, "odd word write %06X\n", address);
    }
    if (address >= 0x200000 && address < 0x210000) {
        uint32_t o = address - 0x200000;
        put_be16(&b->ram[o], value);
        return;
    }
    if (address >= 0x304000 && address < 0x310000) {
        uint32_t o = address - 0x304000;
        put_be16(&b->tile_ram[o], value);
        return;
    }
    if (address >= 0xC00000 && address < 0xC40000) {
        uint32_t o = address - 0xC00000;
        put_be16(&b->spriteram[o], value);
        return;
    }
    if (address >= 0xC40000 && address < 0xC50000) {
        b->palette[(address - 0xC40000) >> 1] = value;
        return;
    }
    if (address >= 0xC50000 && address < 0xC60000) {
        b->extra_palette[(address - 0xC50000) >> 1] = value;
        return;
    }
    if (address >= 0xC60000 && address < 0xC60040) {
        unsigned reg = (address - 0xC60000) >> 1;
        uint16_t old = b->vregs[reg];
        b->vregs[reg] = value;
        if ((reg == 0x24 / 2 || reg == 0x26 / 2) && value && value != old)
            build_sprite_list(b);
        return;
    }
    if (address >= 0xFFFC00 && address < 0x1000000) {
        uint32_t o = address - 0xFFFC00;
        put_be16(&b->tmp[o], value);
        return;
    }
    if (address >= 0xB00000 && address < 0xB04000) {
        uint32_t i = (address - 0xB00000) >> 1;
        b->x1_hi[i] = (uint8_t)(value >> 8);
        b->x1_regs[i] = (uint8_t)value;
        return;
    }
    if (address >= 0xE00010 && address < 0xE00020) {
        b->x1_bank[(address - 0xE00010) & 7] = (uint8_t)value;
        return;
    }
    /* Inputs, watchdog and other word writes with a byte-wide device. */
    if (address >= 0x800000 && address < 0x800002) {
        gr_bus_write8(address + 1, (uint8_t)value);
        return;
    }
    if (address >= 0x600000 && address < 0x700010) return;
    if (address >= 0xE00000 && address < 0xE00010) return;
    bad_write(address);
    (void)value;
}

void gr_bus_write32(uint32_t address, uint32_t value)
{
    gr_bus_write16(address, (uint16_t)(value >> 16));
    gr_bus_write16(address + 2, (uint16_t)value);
}

int gr_board_load(gr_board *board, const char *zip_path)
{
    memset(board, 0, sizeof(*board));
    snprintf(board->rom_path, sizeof(board->rom_path), "%s", zip_path ? zip_path : "");

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, zip_path, 0)) {
        snprintf(board->error, sizeof(board->error), "cannot open ROM archive: %s", zip_path);
        return 0;
    }

    for (size_t i = 0; i < sizeof(rom_files) / sizeof(rom_files[0]); i++) {
        int index = mz_zip_reader_locate_file(&zip, rom_files[i].name, NULL, 0);
        if (index < 0) {
            snprintf(board->error, sizeof(board->error), "ROM archive is missing %s", rom_files[i].name);
            mz_zip_reader_end(&zip);
            return 0;
        }
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, (mz_uint)index, &st) || st.m_uncomp_size != rom_files[i].size) {
            snprintf(board->error, sizeof(board->error), "%s has the wrong size", rom_files[i].name);
            mz_zip_reader_end(&zip);
            return 0;
        }
        if ((mz_uint32)st.m_crc32 != rom_files[i].crc) {
            snprintf(board->error, sizeof(board->error), "%s has CRC32 %08x, expected %08x",
                     rom_files[i].name, (unsigned)st.m_crc32, rom_files[i].crc);
            mz_zip_reader_end(&zip);
            return 0;
        }
    }

    board->program = (uint8_t *)calloc(1, 0x200000);
    board->graphics = (uint8_t *)calloc(1, 0x2000000);
    board->samples = (uint8_t *)calloc(1, 0x100000);
    if (!board->program || !board->graphics || !board->samples) {
        snprintf(board->error, sizeof(board->error), "out of memory loading ROM");
        mz_zip_reader_end(&zip);
        return 0;
    }

    const char *program_names[4] = { "u2.bin", "u3.bin", "u4.bin", "u5.bin" };
    const unsigned program_offsets[4] = { 0x000000, 0x000001, 0x100000, 0x100001 };
    for (int i = 0; i < 4; i++) {
        int index = mz_zip_reader_locate_file(&zip, program_names[i], NULL, 0);
        size_t out_size = 0;
        uint8_t *data = (uint8_t *)mz_zip_reader_extract_to_heap(&zip, (mz_uint)index, &out_size, 0);
        if (!data) { snprintf(board->error, sizeof(board->error), "cannot extract %s", program_names[i]); mz_zip_reader_end(&zip); return 0; }
        for (size_t j = 0; j < out_size; j++) board->program[program_offsets[i] + j * 2] = data[j];
        mz_free(data);
    }

    const char *gfx_names[8] = { "u16.bin", "u15.bin", "u18.bin", "u17.bin",
                                 "u20.bin", "u19.bin", "u22.bin", "u21.bin" };
    const unsigned gfx_offsets[8] = { 0x0000000, 0x0000002, 0x0000004, 0x0000006,
                                      0x1800000, 0x1800002, 0x1800004, 0x1800006 };
    for (int i = 0; i < 8; i++) {
        int index = mz_zip_reader_locate_file(&zip, gfx_names[i], NULL, 0);
        size_t out_size = 0;
        uint8_t *data = (uint8_t *)mz_zip_reader_extract_to_heap(&zip, (mz_uint)index, &out_size, 0);
        if (!data) { snprintf(board->error, sizeof(board->error), "cannot extract %s", gfx_names[i]); mz_zip_reader_end(&zip); return 0; }
        unsigned base = gfx_offsets[i];
        for (size_t j = 0; j + 1 < out_size; j += 2) {
            /* u16/u15/u18/u17 are a single 4 MiB load.  u20/u19/u22/u21 are
             * ROM_LOAD64_WORD 0x200000 followed by ROM_CONTINUE 0x200000. */
            unsigned destination = base + (unsigned)((j / 2) * 8);
            if (i >= 4 && j >= 0x200000)
                destination = base - 0x800000u + (unsigned)(((j - 0x200000) / 2) * 8);
            board->graphics[destination] = data[j];
            board->graphics[destination + 1] = data[j + 1];
        }
        mz_free(data);
    }

    {
        int index = mz_zip_reader_locate_file(&zip, "u32.bin", NULL, 0);
        size_t out_size = 0;
        uint8_t *data = (uint8_t *)mz_zip_reader_extract_to_heap(&zip, (mz_uint)index, &out_size, 0);
        if (!data) { snprintf(board->error, sizeof(board->error), "cannot extract u32.bin"); mz_zip_reader_end(&zip); return 0; }
        memcpy(board->samples, data, out_size < 0x100000 ? out_size : 0x100000);
        mz_free(data);
    }

    mz_zip_reader_end(&zip);
    gr_board_reset(board);
    return 1;
}

void gr_board_free(gr_board *board)
{
    free(board->program); board->program = NULL;
    free(board->graphics); board->graphics = NULL;
    free(board->samples); board->samples = NULL;
    if (g_board == board) g_board = NULL;
}

void gr_board_reset(gr_board *board)
{
    uint32_t sp = (uint32_t)board->program[0] << 24 | (uint32_t)board->program[1] << 16 |
                  (uint32_t)board->program[2] << 8 | board->program[3];
    uint32_t pc = (uint32_t)board->program[4] << 24 | (uint32_t)board->program[5] << 16 |
                  (uint32_t)board->program[6] << 8 | board->program[7];
    memset(&board->cpu, 0, sizeof(board->cpu));
    board->cpu.a[7] = sp;
    board->cpu.pc = pc & 0xFFFFFF;
    board->cpu.sr = 0x2700;
    memset(board->ram, 0, sizeof(board->ram));
    memset(board->tile_ram, 0, sizeof(board->tile_ram));
    memset(board->spriteram, 0, sizeof(board->spriteram));
    memset(board->palette, 0, sizeof(board->palette));
    memset(board->extra_palette, 0, sizeof(board->extra_palette));
    memset(board->vregs, 0, sizeof(board->vregs));
    memset(board->x1_regs, 0, sizeof(board->x1_regs));
    memset(board->x1_hi, 0, sizeof(board->x1_hi));
    memset(board->tmp, 0, sizeof(board->tmp));
    board->tmp[0x01] = 0xFF; board->tmp[0x05] = 0xFF;
    board->tmp[0x03] = 0x3D; board->tmp[0x07] = 0x18; board->tmp[0x09] = 0x18;
    board->tmp[0x0B] = 0x08;
    for (int i = 0; i < 10; i++) board->tmp[0x80 + i * 2 + 1] = 0x07;
    board->tmp[0x94] = 0x07; board->tmp[0x95] = 0xF7;
    board->tmp[0x9B] = 0x00;
    board->inputs = 0;
    board->frame_counter = 0;
    board->raster_line = 0;
    board->sound_enabled = 1;
    gr_bus_statistics.reads = gr_bus_statistics.writes = 0;
    gr_bus_statistics.unmapped_reads = gr_bus_statistics.unmapped_writes = 0;
    gr_bus_statistics.odd_word_accesses = 0;
    g_board = board;
}

void gr_board_raise_vblank(gr_board *board)
{
    board->frame_counter++;
    if (board->cpu.irq_pending) return;
    board->cpu.irq_pending = 1;
    board->cpu.irq_level = 4;
    board->cpu.irq_vector = 0x40; /* IVNR 0x40: MAME tmp68301 interrupt_callback */
}

void gr_board_raise_raster(gr_board *board, uint32_t line)
{
    board->raster_line = line;
    if (board->cpu.irq_pending) return;
    board->cpu.irq_pending = 1;
    board->cpu.irq_level = 4;
    board->cpu.irq_vector = 0x41;
}

static uint16_t spr_get(const gr_board *b, unsigned word)
{
    unsigned byte = (word & 0x1FFFFu) * 2u;
    return (uint16_t)((b->spriteram[byte] << 8) | b->spriteram[byte + 1]);
}

static void spr_put(gr_board *b, unsigned word, uint16_t value)
{
    unsigned byte = (word & 0x1FFFFu) * 2u;
    b->spriteram[byte] = (uint8_t)(value >> 8);
    b->spriteram[byte + 1] = (uint8_t)value;
}

/* MAME x1_020_dx_101.cpp, vregs_w case 0x24/0x26: copy the list at 0x3000
 * into a private buffer, flattening the indexed single-sprite lists. */
static void build_sprite_list(gr_board *board)
{
    uint16_t *priv = board->sprite_private;
    int current = 0;
    for (int i = 0; i < 0x800; i += 4) {
        uint16_t num = spr_get(board, 0x1800u + (unsigned)i);
        priv[i + 0] = num;
        priv[i + 1] = spr_get(board, 0x1801u + (unsigned)i);
        priv[i + 2] = spr_get(board, 0x1802u + (unsigned)i);
        int sprite = spr_get(board, 0x1803u + (unsigned)i);
        priv[i + 3] = (uint16_t)(((current / 4) & 0x7FFF) | (sprite & 0x8000));
        int list2addr = (sprite & 0x7FFF) * 4;
        num &= 0xFF;
        for (int j = 0; j <= num; j++) {
            if (current < 0x1800) {
                for (int k = 0; k < 4; k++)
                    spr_put(board, (unsigned)(current + k),
                            spr_get(board, (unsigned)(list2addr + j * 4 + k)));
                current += 4;
            }
        }
        if (priv[i] & 0x8000) {
            if (sprite == 0) priv[i + 3] |= 0x4000;
            break;
        }
    }
}

/* Decode one 8x8, 8bpp tile from the 64-KiB gfx region.  MAME's tile_layout:
 * plane offsets 56,48,...,0; each plane has 8 rows of 8 bytes; bit 7 is x=0. */
static unsigned tile_pen(const gr_board *b, unsigned code, int x, int y,
                         int flipx, int flipy)
{
    if (flipx) x = 7 - x;
    if (flipy) y = 7 - y;
    unsigned index = code % (0x2000000u / 64u);
    const uint8_t *tile = b->graphics + (size_t)index * 64u;
    unsigned pen = 0;
    for (int plane = 0; plane < 8; plane++) {
        unsigned offset = (unsigned)(7 - plane) + (unsigned)y * 8u;
        pen |= ((tile[offset] >> (7 - x)) & 1u) << (7 - plane);
    }
    return pen;
}

static void draw_tile_line(gr_board *board, uint16_t *pixels, int width, int height,
                           int screenline, unsigned code, unsigned color,
                           int sx, int line, int flipx, int flipy, int opaque,
                           int which_gfx)
{
    static const struct { int mask, shift; } bpp[8] = {
        { 0xFF, 0 }, { 0x30, 4 }, { 0x07, 0 }, { 0xFF, 0 },
        { 0x0F, 0 }, { 0xF0, 4 }, { 0x3F, 0 }, { 0xFF, 0 },
    };
    if (screenline < 0 || screenline >= height) return;
    if (which_gfx < 0 || which_gfx > 7) which_gfx = 7;
    for (int px = 0; px < 8; px++) {
        int dx = sx + px;
        if (dx < 0 || dx >= width) continue;
        unsigned pen = tile_pen(board, code, px, line, flipx, flipy);
        pen = (pen & (unsigned)bpp[which_gfx].mask) >> bpp[which_gfx].shift;
        if (pen || opaque)
            pixels[(size_t)screenline * (size_t)width + (size_t)dx] = (uint16_t)((color + pen) & 0x7FFF);
    }
}

static void draw_sprite_line(gr_board *board, uint16_t *pixels, int width, int height,
                             int scanline, int realscanline, int xoffset,
                             uint32_t xzoom, int xzoominverted)
{
    (void)xzoominverted;
    const uint16_t *s1 = board->sprite_private;
    for (int entry = 0; entry < 0x800; entry += 4, s1 += 4) {
        int num = s1[0];
        int xoffs = s1[1];
        int yoffs = s1[2];
        int sprite = s1[3];
        unsigned s2word = (unsigned)(sprite & 0x7FFF) * 4u;

        int global_sizex = xoffs & 0xFC00;
        int global_sizey = yoffs & 0xFC00;
        int nozoom = (num >> 14) & 1;
        int opaque = (num >> 13) & 1;
        int use_global_size = (num >> 12) & 1;
        int which_gfx = (num & 0x0700) >> 8;
        xoffs &= 0x3FF;
        yoffs &= 0x3FF;
        if (yoffs & 0x200) yoffs -= 0x400;

        int global_xoffset = nozoom ? 0x80 : 0;
        int global_yoffset = nozoom ? -0x90 : 0;
        int usedscanline = nozoom ? realscanline : scanline;
        int usedxoffset = nozoom ? 0 : (xoffset >> 16);
        uint32_t usedxzoom = nozoom ? 0x10000u : xzoom;

        num = (num & 0x00FF) + 1;
        if ((sprite & 0x7FFF) >= 0x1800) {
            if (s1[0] & 0x8000) break;
            continue;
        }

        for (; num > 0; num--, s2word += 4u) {
            if (s2word >= 0x20000u) break;
            if (sprite & 0x8000) {
                /* Floating tilemap window (MAME draw_sprites_line). */
                int sy = (int)(spr_get(board, s2word + 1u) & 0x3FF);
                sy += global_yoffset;
                sy &= 0x3FF;
                if (sy & 0x200) sy -= 0x400;
                int local_sizey = spr_get(board, s2word + 1u) & 0xFC00;
                int h = use_global_size ? global_sizey : local_sizey;
                h = ((h & 0xFC00) >> 10) + 1;
                int firstline = (sy + yoffs) & 0x3FF;
                if (firstline & 0x200) firstline -= 0x400;
                int endline = firstline + h * 0x10 - 1;
                if (endline & 0x200) endline -= 0x400;
                if (endline >= firstline) {
                    if (firstline > usedscanline || endline < usedscanline) continue;
                } else if (usedscanline > endline && usedscanline < firstline) continue;

                int sx = spr_get(board, s2word + 0u);
                int local_sizex = sx & 0xFC00;
                sx &= 0x3FF;
                sx -= global_xoffset;
                int w = use_global_size ? global_sizex : local_sizex;
                w = (w & 0xFC00) >> 10;
                if (!w) continue;
                int firstcolumn = (sx + xoffs);
                firstcolumn = (firstcolumn & 0x1FF) - (firstcolumn & 0x200);
                int lastcolumn = firstcolumn + w * 0x10 - 1;
                if (lastcolumn < 0 || firstcolumn >= width) continue;

                int scrolly = (int)(spr_get(board, s2word + 3u) & 0x1FF);
                scrolly += global_yoffset;
                int sourceline = (usedscanline - scrolly) & 0x1FF;
                int scrollx = spr_get(board, s2word + 2u);
                int is16 = (scrollx >> 15) & 1;
                int page = (scrollx & 0x7C00) >> 10;
                scrollx &= 0x3FF;
                for (int x = 0; x < 0x80; x++) {
                    int xtile = x * 8;
                    int ytile = sourceline;
                    int code, attr, color;
                    int flipx, flipy;
                    /* MAME get_tile: x and y are in pixels and reduced to
                     * tile coordinates (8x8, or 16x16 for is16). */
                    xtile >>= is16 ? 4 : 3;
                    ytile >>= is16 ? 4 : 3;
                    unsigned base = (page * 0x2000u) / 2u;
                    unsigned off = base + (unsigned)(((ytile & 0x1F) << 6) + (xtile & 0x3F)) * 2u;
                    uint16_t raw_attr = spr_get(board, off);
                    uint16_t raw_code = spr_get(board, off + 1u);
                    attr = raw_attr;
                    code = raw_code + ((attr & 7) << 16);
                    flipx = (attr >> 4) & 1;
                    flipy = (attr >> 3) & 1;
                    color = (attr & 0xFFE0) >> 5;
                    if (is16) code &= ~3;
                    int tileline = sourceline & 7;
                    int dx = sx + (scrollx & 0x3FF) + xoffs + 0x10;
                    int px = (((dx + x * 8) + 0x10) & 0x3FF) - 0x10;
                    int dst_x = px & 0x3FF;
                    dst_x = (dst_x & 0x1FF) - (dst_x & 0x200);
                    if (dst_x >= firstcolumn - 8 && dst_x <= lastcolumn) {
                        int realsx = dst_x - usedxoffset;
                        if (usedxzoom != 0) realsx = (int)((int64_t)realsx * (int64_t)usedxzoom >> 16);
                        draw_tile_line(board, pixels, width, height, realscanline,
                                       (unsigned)code, (unsigned)color << 4,
                                       realsx, tileline, flipx, flipy, opaque, which_gfx);
                    }
                }
                continue;
            }

            /* Normal sprite. */
            int sy = spr_get(board, s2word + 1u) & 0x1FF;
            if (sy & 0x100) sy -= 0x200;
            sy += global_yoffset;
            sy &= 0x3FF;
            int sizey = use_global_size ? global_sizey : (spr_get(board, s2word + 1u) & 0xFC00);
            sizey = (1 << ((sizey & 0x0C00) >> 10)) - 1;
            int firstline = (sy + yoffs) & 0x3FF;
            int endline = (firstline + (sizey + 1) * 8) - 1;
            endline &= 0x3FF;
            if (firstline & 0x200) firstline -= 0x400;
            if (endline & 0x200) endline -= 0x400;
            if (endline >= firstline) {
                if (firstline > usedscanline || endline < usedscanline) continue;
            } else if (usedscanline > endline && usedscanline < firstline) continue;

            int attr = spr_get(board, s2word + 2u);
            int code = spr_get(board, s2word + 3u) + ((attr & 7) << 16);
            int flipx = (attr >> 4) & 1;
            int flipy = (attr >> 3) & 1;
            int color = (attr & 0xFFE0) >> 5;
            int sx = spr_get(board, s2word + 0u);
            int sizex = use_global_size ? global_sizex : sx;
            sizex = (1 << ((sizex & 0x0C00) >> 10)) - 1;
            sx += xoffs;
            sx = (sx & 0x1FF) - (sx & 0x200);
            sx -= global_xoffset;
            int line = usedscanline - firstline;
            int y = line >> 3;
            line &= 7;
            if (nozoom) color = 0x7FF;
            int basecode = (code & ~((sizex + 1) * (sizey + 1) - 1)) +
                           ((flipy ? (sizey - y) : y) * (sizex + 1)) + (flipx ? sizex : 0);
            int code_inc = flipx ? -1 : 1;
            for (int x = 0; x <= sizex; x++) {
                int realsx = sx + x * 8 - usedxoffset;
                if (usedxzoom != 0) realsx = (int)((int64_t)realsx * (int64_t)usedxzoom >> 16);
                draw_tile_line(board, pixels, width, height, realscanline,
                               (unsigned)basecode, (unsigned)color << 4,
                               realsx, line, flipx, flipy, opaque, which_gfx);
                basecode += code_inc;
            }
        }
        if (s1[0] & 0x8000) break;
    }
}

void gr_board_render(gr_board *board, uint16_t *pixels, int width, int height)
{
    /* The guest builds its display list during the frame; refresh our private
       copy at presentation time so the draw sees the list the CPU just wrote. */
    build_sprite_list(board);
    uint16_t background = board->palette[0];
    for (int i = 0; i < width * height; i++) pixels[i] = background;

    if ((~board->vregs[0x30 / 2]) & 1u) {
        uint32_t yoffset = ((uint32_t)board->vregs[0x1A / 2] << 16) | board->vregs[0x18 / 2];
        yoffset &= 0x07FFFFFFu;
        yoffset = 0x07FFFFFFu - yoffset;
        uint32_t yzoom = ((uint32_t)board->vregs[0x1E / 2] << 16) | board->vregs[0x1C / 2];
        yzoom &= 0x07FFFFFFu;
        int yzoominverted = 0;
        if (yzoom & 0x04000000u) { yzoom = 0x08000000u - yzoom; yzoominverted = 1; }
        int xoffset = (int)(((uint32_t)board->vregs[0x12 / 2] << 16) | board->vregs[0x10 / 2]);
        xoffset &= 0x07FFFFFF;
        if (xoffset & 0x04000000) xoffset -= 0x08000000;
        uint32_t xzoom = ((uint32_t)board->vregs[0x16 / 2] << 16) | board->vregs[0x14 / 2];
        int xzoominverted = 0;
        if (xzoom & 0x04000000u) { xzoom = 0x08000000u - xzoom; xzoominverted = 1; }
        if (!xzoom) return;
        uint32_t inc2 = (uint32_t)(0x100000000ull / xzoom);
        for (int y = 0; y < height; y++) {
            int yy = y;
            if (yzoominverted) {
                yy = (int)(((uint32_t)y * yzoom + yoffset) & 0x07FFFFFFu);
                yy >>= 16;
                if (yy & 0x400) yy -= 0x800;
            }
            draw_sprite_line(board, pixels, width, height, yy, y, xoffset, inc2, xzoominverted);
        }
    }
}

void gr_board_set_inputs(gr_board *board, uint32_t active_high)
{
    board->inputs = active_high;
}

const char *gr_board_last_error(const gr_board *board)
{
    return board->error;
}
