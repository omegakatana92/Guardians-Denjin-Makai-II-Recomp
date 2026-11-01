/* Deterministic headless runner: ROM -> native translated program -> hashes. */
#include "grdians/board.h"
#include "grdians/bus.h"
#include "m68k/runner.h"
#include "program.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t fnv1a(uint64_t hash, const void *data, size_t length)
{
    const uint8_t *p = (const uint8_t *)data;
    for (size_t i = 0; i < length; i++) {
        hash ^= p[i];
        hash *= 0x100000001B3ull;
    }
    return hash;
}

static void usage(const char *program)
{
    fprintf(stderr,
            "usage: %s --rom FILE.zip [--frames N] [--budget N] [--hashes FILE]\n"
            "          [--png FILE] [--dump-ram FILE]\n", program);
}

int main(int argc, char **argv)
{
    const char *rom = NULL;
    const char *hashes = NULL;
    const char *png = NULL;
    const char *dump_ram = NULL;
    const char *dump_sprites = NULL;
    const char *dump_private = NULL;
    const char *dump_graphics = NULL;
    const char *dump_palette = NULL;
    int frames = 600;
    uint64_t budget = 35000;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom") && i + 1 < argc) rom = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--budget") && i + 1 < argc) budget = strtoull(argv[++i], NULL, 0);
        else if (!strcmp(argv[i], "--hashes") && i + 1 < argc) hashes = argv[++i];
        else if (!strcmp(argv[i], "--png") && i + 1 < argc) png = argv[++i];
        else if (!strcmp(argv[i], "--dump-ram") && i + 1 < argc) dump_ram = argv[++i];
        else if (!strcmp(argv[i], "--dump-sprites") && i + 1 < argc) dump_sprites = argv[++i];
        else if (!strcmp(argv[i], "--dump-private") && i + 1 < argc) dump_private = argv[++i];
        else if (!strcmp(argv[i], "--dump-graphics") && i + 1 < argc) dump_graphics = argv[++i];
        else if (!strcmp(argv[i], "--dump-palette") && i + 1 < argc) dump_palette = argv[++i];
        else if (!strcmp(argv[i], "--strict")) gr_bus_strict = 1;
        else { usage(argv[0]); return 2; }
    }
    if (!rom) {
        rom = getenv("GRDIANS_ROM");
        if (!rom) rom = "C:/Users/omega/Downloads/grdians.zip";
    }

    gr_bus_strict = 0;
    gr_board board;
    if (!gr_board_load(&board, rom)) {
        fprintf(stderr, "error: %s\n", gr_board_last_error(&board));
        return 1;
    }
    gr_board_reset(&board);
    m68k_program_init();

    printf("ROM: %s\n", rom);
    printf("program: reset PC %06X, SP %08X, %u translated entries\n",
           board.cpu.pc, board.cpu.a[7], m68k_program_entry_count());

    FILE *hash_file = hashes ? fopen(hashes, "w") : NULL;
    if (hashes && !hash_file) {
        fprintf(stderr, "error: cannot write %s\n", hashes);
        return 1;
    }
    if (hash_file)
        fprintf(hash_file, "# frame pc ram sprites palette vregs instructions\n");

    uint16_t *pixels = (uint16_t *)calloc(GR_SCREEN_WIDTH * GR_SCREEN_HEIGHT, sizeof(uint16_t));
    if (!pixels) {
        fprintf(stderr, "error: out of memory\n");
        return 1;
    }

    int failed = 0;
    for (int frame = 0; frame < frames; frame++) {
        gr_board_raise_vblank(&board);
        int result = m68k_run(&board.cpu, budget);
        if (result != M68K_RUN_OK) {
            fprintf(stderr, "stopped at frame %d: no translated code at %06X\n",
                    frame, m68k_last_missing_pc());
            failed = 1;
            break;
        }

        gr_board_render(&board, pixels, GR_SCREEN_WIDTH, GR_SCREEN_HEIGHT);

        uint64_t h_ram = fnv1a(0xcbf29ce484222325ull, board.ram, sizeof(board.ram));
        uint64_t h_spr = fnv1a(0xcbf29ce484222325ull, board.spriteram, sizeof(board.spriteram));
        uint64_t h_pal = fnv1a(0xcbf29ce484222325ull, board.palette, sizeof(board.palette));
        uint64_t h_vreg = fnv1a(0xcbf29ce484222325ull, board.vregs, sizeof(board.vregs));
        if (hash_file) {
            fprintf(hash_file, "%d %06X %016" PRIx64 " %016" PRIx64 " %016" PRIx64
                    " %016" PRIx64 " %" PRIu64 "\n",
                    frame, board.cpu.pc, h_ram, h_spr, h_pal, h_vreg, board.cpu.instructions);
        }
        if (frame % 60 == 0 || frame == frames - 1)
            printf("frame %5d pc %06X ram %016" PRIx64 " sprites %016" PRIx64
                   " instr %" PRIu64 "\n",
                   frame, board.cpu.pc, h_ram, h_spr, board.cpu.instructions);
    }

    if (!failed && png) {
        if (!gr_board_save_png(png, pixels, GR_SCREEN_WIDTH, GR_SCREEN_HEIGHT)) {
            fprintf(stderr, "error: cannot write %s\n", png);
            failed = 1;
        } else {
            printf("saved %s (%dx%d)\n", png, GR_SCREEN_WIDTH, GR_SCREEN_HEIGHT);
        }
    }
    if (dump_ram) {
        FILE *f = fopen(dump_ram, "wb");
        if (!f || fwrite(board.ram, 1, sizeof(board.ram), f) != sizeof(board.ram)) {
            fprintf(stderr, "error: cannot write %s\n", dump_ram);
            failed = 1;
        }
        if (f) fclose(f);
    }

    if (dump_sprites) {
        FILE *f = fopen(dump_sprites, "wb");
        if (f) { fwrite(board.spriteram, 1, sizeof(board.spriteram), f); fclose(f); }
    }
    if (dump_palette) {
        FILE *f = fopen(dump_palette, "wb");
        if (f) { fwrite(board.palette, 1, sizeof(board.palette), f); fclose(f); }
    }
    if (dump_graphics) {
        FILE *f = fopen(dump_graphics, "wb");
        if (f) { fwrite(board.graphics, 1, 0x2000000, f); fclose(f); }
    }
    if (dump_private) {
        FILE *f = fopen(dump_private, "wb");
        if (f) { fwrite(board.sprite_private, 1, sizeof(board.sprite_private), f); fclose(f); }
    }

    if (hash_file) fclose(hash_file);
    printf("bus: reads %" PRIu64 " writes %" PRIu64 " unmapped reads %" PRIu64
           " unmapped writes %" PRIu64 " odd words %" PRIu64 "\n",
           gr_bus_statistics.reads, gr_bus_statistics.writes,
           gr_bus_statistics.unmapped_reads, gr_bus_statistics.unmapped_writes,
           gr_bus_statistics.odd_word_accesses);
    printf("CPU: %" PRIu64 " instructions, %" PRIu64 " exceptions\n",
           board.cpu.instructions, board.cpu.exceptions);
    free(pixels);
    gr_board_free(&board);
    return failed ? 1 : 0;
}
