#include "m68k/runner.h"
#include "program.h"

#include <stdio.h>

static uint32_t last_missing;
static uint64_t missing_count;

static uint32_t history[64];
static unsigned history_at;

int m68k_run(m68k_state *s, uint64_t budget)
{
    for (uint64_t step = 0; step < budget; step++) {
        history[history_at++ & 63u] = s->pc;
        if (s->irq_pending) {
            unsigned mask = (s->sr & 0x0700u) >> 8;
            if (s->irq_level == 7 || mask < s->irq_level) {
                m68k_interrupt(s);
                s->instructions++;
                continue;
            }
            if (s->stopped)
                return M68K_RUN_OK;
        }
        if (s->stopped)
            return M68K_RUN_OK;

        uint32_t next = m68k_program_step(s, s->pc);
        if (next == 0xFFFFFFFFu) {
            last_missing = s->pc;
            missing_count++;
            if (missing_count <= 32) {
                fprintf(stderr, "missing translated code at %06X (after %llu instructions)\n",
                        s->pc, (unsigned long long)s->instructions);
                fprintf(stderr, "last PCs:");
                for (unsigned i = 0; i < 24; i++)
                    fprintf(stderr, " %06X", history[(history_at - 1u - i) & 63u]);
                fprintf(stderr, "\nregisters: D0=%08X D1=%08X D2=%08X D3=%08X "
                        "A0=%08X A1=%08X A2=%08X A3=%08X A4=%08X A5=%08X A6=%08X A7=%08X\n",
                        s->d[0], s->d[1], s->d[2], s->d[3],
                        s->a[0], s->a[1], s->a[2], s->a[3],
                        s->a[4], s->a[5], s->a[6], s->a[7]);
            }
            return M68K_RUN_MISSING;
        }
        s->pc = next;
        s->instructions++;
    }
    return M68K_RUN_OK;
}

uint32_t m68k_last_missing_pc(void)
{
    return last_missing;
}

uint64_t m68k_missing_count(void)
{
    return missing_count;
}
