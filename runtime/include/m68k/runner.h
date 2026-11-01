/* Top-level dispatch loop for the statically translated program. */
#ifndef GRDIANS_RUNNER_H
#define GRDIANS_RUNNER_H

#include <stdint.h>
#include "m68k/m68k.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M68K_RUN_OK 0
#define M68K_RUN_MISSING (-1)

/* Run at most 'budget' instructions.  Interrupts are delivered between
 * instructions when the SR mask allows them.  Returns M68K_RUN_MISSING when
 * execution reaches an address with no translated function. */
int m68k_run(m68k_state *s, uint64_t budget);
uint32_t m68k_last_missing_pc(void);
uint64_t m68k_missing_count(void);

#ifdef __cplusplus
}
#endif
#endif
