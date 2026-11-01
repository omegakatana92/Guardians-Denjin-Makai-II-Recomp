/* Seta 2 board model for Guardians: Denjin Makai II.
 *
 * Reference: MAME 0.289 src/mame/seta/seta2.cpp (grdians_map, grdians,
 * x1_020_dx_101, x1_010) and src/devices/cpu/m68000/tmp68301.cpp.
 */
#ifndef GRDIANS_BOARD_H
#define GRDIANS_BOARD_H

#include <stddef.h>
#include <stdint.h>
#include "m68k/m68k.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GR_SCREEN_WIDTH  0x130
#define GR_SCREEN_HEIGHT 0xE8
#define GR_SCREEN_MAX_WIDTH  0x200
#define GR_SCREEN_MAX_HEIGHT 0x100

enum {
    GR_INPUT_P1_LEFT  = 0x0001,
    GR_INPUT_P1_RIGHT = 0x0002,
    GR_INPUT_P1_UP    = 0x0004,
    GR_INPUT_P1_DOWN  = 0x0008,
    GR_INPUT_P1_B1    = 0x0010,
    GR_INPUT_P1_B2    = 0x0020,
    GR_INPUT_P1_B3    = 0x0040,
    GR_INPUT_P1_START = 0x0080,
    GR_INPUT_P2_LEFT  = 0x0100,
    GR_INPUT_P2_RIGHT = 0x0200,
    GR_INPUT_P2_UP    = 0x0400,
    GR_INPUT_P2_DOWN  = 0x0800,
    GR_INPUT_P2_B1    = 0x1000,
    GR_INPUT_P2_B2    = 0x2000,
    GR_INPUT_P2_B3    = 0x4000,
    GR_INPUT_P2_START = 0x8000,
    GR_INPUT_COIN1    = 0x10000,
    GR_INPUT_COIN2    = 0x20000,
    GR_INPUT_SERVICE  = 0x40000,
    GR_INPUT_TEST     = 0x80000
};

typedef struct gr_board {
    m68k_state cpu;

    uint8_t *program;      /* 2 MiB */
    uint8_t *graphics;     /* 32 MiB, MAME `video` region */
    uint8_t *samples;      /* 1 MiB */

    uint8_t ram[0x10000];          /* 0x200000 */
    uint8_t tile_ram[0xC000];      /* 0x304000 (MAME comment: tile data) */
    uint8_t spriteram[0x40000];    /* 0xC00000 */
    uint16_t sprite_private[0x800];/* copy/reformat of the list at 0x3000 */
    uint16_t palette[0x8000];      /* 0xC40000, xRGB_555 */
    uint16_t extra_palette[0x8000];/* 0xC50000 */
    uint16_t vregs[0x20];          /* 0xC60000 */
    uint8_t x1_regs[0x2000];       /* 0xB00000, X1-010 low bytes */
    uint8_t x1_hi[0x2000];         /* high-byte latch of word_w */
    uint8_t x1_bank[8];
    uint8_t tmp[0x400];            /* 0xFFFC00 TMP68301 internal registers */

    uint32_t inputs;               /* active-high logical button state */
    uint32_t frame_counter;
    uint32_t raster_line;
    int sound_enabled;
    int coin_counter[2];

    char rom_path[1024];
    char error[512];
} gr_board;

int  gr_board_load(gr_board *board, const char *zip_path);
void gr_board_free(gr_board *board);
void gr_board_reset(gr_board *board);
void gr_board_run(gr_board *board, uint64_t instruction_budget);
void gr_board_raise_vblank(gr_board *board);
void gr_board_raise_raster(gr_board *board, uint32_t line);
void gr_board_render(gr_board *board, uint16_t *pixels, int width, int height);
void gr_board_set_inputs(gr_board *board, uint32_t active_high);
int  gr_board_save_png(const char *path, const uint16_t *pixels, int width, int height);

const char *gr_board_last_error(const gr_board *board);

#ifdef __cplusplus
}
#endif
#endif
