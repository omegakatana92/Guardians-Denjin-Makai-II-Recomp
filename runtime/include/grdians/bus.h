/* Seta 2 / Guardians board bus.
 *
 * Reference: MAME src/mame/seta/seta2.cpp, seta2_state::grdians_map() (0.289).
 * The TMP68301 internal peripherals live at 0xfffc00-0xffffff.
 */
#ifndef GRDIANS_BUS_H
#define GRDIANS_BUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t  gr_bus_read8(uint32_t address);
uint16_t gr_bus_read16(uint32_t address);
uint32_t gr_bus_read32(uint32_t address);
void     gr_bus_write8(uint32_t address, uint8_t value);
void     gr_bus_write16(uint32_t address, uint16_t value);
void     gr_bus_write32(uint32_t address, uint32_t value);

/* Diagnostics. */
typedef struct gr_bus_stats {
    uint64_t reads;
    uint64_t writes;
    uint64_t unmapped_reads;
    uint64_t unmapped_writes;
    uint64_t odd_word_accesses;
    uint32_t last_bad_read;
    uint32_t last_bad_write;
} gr_bus_stats;

extern gr_bus_stats gr_bus_statistics;
extern int gr_bus_strict;

#ifdef __cplusplus
}
#endif
#endif
