/* Motorola 68000 execution state and helpers used by the translated program.
 *
 * The translated C never fetches or decodes an instruction: every call here is
 * emitted at build time by tools/recompile68k.py.  The semantics follow the
 * MC68000 User's Manual (condition codes, byte/word write behaviour, exception
 * frames) and MAME's Musashi core for the details the manual leaves open.
 */
#ifndef GRDIANS_M68K_H
#define GRDIANS_M68K_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    M68K_EA_DREG = 0,
    M68K_EA_AREG,
    M68K_EA_IND,
    M68K_EA_POSTINC,
    M68K_EA_PREDEC,
    M68K_EA_DISP,
    M68K_EA_INDEX,
    M68K_EA_ABS,
    M68K_EA_ABS_INDEX,
    M68K_EA_IMM
};

/* An effective address.  For M68K_EA_INDEX, index holds the index register
 * number in the low nibble and (1 << 4) for a long index.  For ABS/ABS_INDEX,
 * value is the absolute base address. */
typedef struct m68k_ea {
    uint8_t mode;
    uint8_t reg;
    uint8_t index;
    int32_t disp;
    uint32_t value;
} m68k_ea;

#define EA_DREG(n) ((m68k_ea){ .mode = M68K_EA_DREG, .reg = (uint8_t)(n) })
#define EA_AREG(n) ((m68k_ea){ .mode = M68K_EA_AREG, .reg = (uint8_t)(n) })
#define EA_IND(n) ((m68k_ea){ .mode = M68K_EA_IND, .reg = (uint8_t)(n) })
#define EA_POST(n) ((m68k_ea){ .mode = M68K_EA_POSTINC, .reg = (uint8_t)(n) })
#define EA_PRE(n) ((m68k_ea){ .mode = M68K_EA_PREDEC, .reg = (uint8_t)(n) })
#define EA_DISP(n, d) ((m68k_ea){ .mode = M68K_EA_DISP, .reg = (uint8_t)(n), .disp = (int32_t)(d) })
#define EA_INDEX(n, idx, long_index, d) \
    ((m68k_ea){ .mode = M68K_EA_INDEX, .reg = (uint8_t)(n), \
                .index = (uint8_t)((idx) | ((long_index) ? 0x10 : 0)), .disp = (int32_t)(d) })
#define EA_ABS(a) ((m68k_ea){ .mode = M68K_EA_ABS, .value = (uint32_t)(a) })
#define EA_ABS_INDEX(a, idx, long_index) \
    ((m68k_ea){ .mode = M68K_EA_ABS_INDEX, \
                .index = (uint8_t)((idx) | ((long_index) ? 0x10 : 0)), .value = (uint32_t)(a) })
#define EA_IMM(v) ((m68k_ea){ .mode = M68K_EA_IMM, .value = (uint32_t)(v) })

enum {
    M68K_SHIFT_ASL = 0, M68K_SHIFT_ASR, M68K_SHIFT_LSL, M68K_SHIFT_LSR,
    M68K_SHIFT_ROL, M68K_SHIFT_ROR, M68K_SHIFT_ROXL, M68K_SHIFT_ROXR
};
enum { M68K_BIT_TST = 0, M68K_BIT_CHG, M68K_BIT_CLR, M68K_BIT_SET };

typedef struct m68k_state {
    uint32_t d[8];
    uint32_t a[8];
    uint32_t pc;
    uint16_t sr;
    uint8_t stopped;
    uint8_t irq_pending;
    uint8_t irq_level;
    uint8_t irq_vector;
    uint64_t instructions;
    uint64_t exceptions;
} m68k_state;

/* Condition codes in SR. */
enum {
    M68K_C = 0x0001, M68K_V = 0x0002, M68K_Z = 0x0004, M68K_N = 0x0008,
    M68K_X = 0x0010, M68K_IPL = 0x0700, M68K_S = 0x2000, M68K_T = 0x8000
};

uint32_t m68k_get(m68k_state *s, m68k_ea ea, int size);
void m68k_put(m68k_state *s, m68k_ea ea, int size, uint32_t value);
uint32_t m68k_ea_address(m68k_state *s, m68k_ea ea);
uint32_t m68k_ea_address_indexed(m68k_state *s, m68k_ea ea);

void m68k_push32(m68k_state *s, uint32_t value);
uint32_t m68k_pop32(m68k_state *s);
void m68k_push16(m68k_state *s, uint16_t value);
uint16_t m68k_pop16(m68k_state *s);

void m68k_set_sr(m68k_state *s, uint32_t value);
void m68k_set_ccr(m68k_state *s, uint32_t value);
void m68k_move_from_sr(m68k_state *s, m68k_ea dst, int size);
void m68k_move_from_ccr(m68k_state *s, m68k_ea dst, int size);
void m68k_ori_ccr(m68k_state *s, uint32_t value);
void m68k_andi_ccr(m68k_state *s, uint32_t value);
void m68k_eori_ccr(m68k_state *s, uint32_t value);

int m68k_cond(const m68k_state *s, int condition);
int m68k_dbcc(m68k_state *s, int reg, int condition, uint32_t target);

void m68k_move(m68k_state *s, m68k_ea src, m68k_ea dst, int size);
void m68k_movea(m68k_state *s, m68k_ea src, int areg, int size);
void m68k_moveq(m68k_state *s, int32_t value, int dreg);
void m68k_move_to_sr(m68k_state *s, m68k_ea src, int size);
void m68k_move_to_ccr(m68k_state *s, m68k_ea src, int size);

void m68k_add(m68k_state *s, m68k_ea src, m68k_ea dst, int size, int with_x);
void m68k_sub(m68k_state *s, m68k_ea src, m68k_ea dst, int size, int with_x);
void m68k_adda(m68k_state *s, m68k_ea src, int areg, int size);
void m68k_suba(m68k_state *s, m68k_ea src, int areg, int size);
void m68k_addi(m68k_state *s, uint32_t value, m68k_ea dst, int size);
void m68k_subi(m68k_state *s, uint32_t value, m68k_ea dst, int size);
void m68k_addq(m68k_state *s, uint32_t value, m68k_ea dst, int size);
void m68k_subq(m68k_state *s, uint32_t value, m68k_ea dst, int size);
void m68k_addx(m68k_state *s, m68k_ea src, int dreg, int size);
void m68k_subx(m68k_state *s, m68k_ea src, int dreg, int size);
void m68k_cmp(m68k_state *s, m68k_ea src, m68k_ea dst, int size);
void m68k_cmpa(m68k_state *s, m68k_ea src, int areg, int size);
void m68k_cmpi(m68k_state *s, uint32_t value, m68k_ea dst, int size);

void m68k_and(m68k_state *s, m68k_ea src, m68k_ea dst, int size);
void m68k_or(m68k_state *s, m68k_ea src, m68k_ea dst, int size);
void m68k_eor(m68k_state *s, m68k_ea src, m68k_ea dst, int size);
void m68k_andi(m68k_state *s, uint32_t value, m68k_ea dst, int size);
void m68k_ori(m68k_state *s, uint32_t value, m68k_ea dst, int size);
void m68k_eori(m68k_state *s, uint32_t value, m68k_ea dst, int size);

void m68k_clr(m68k_state *s, m68k_ea dst, int size);
void m68k_tst(m68k_state *s, m68k_ea dst, int size);
void m68k_neg(m68k_state *s, m68k_ea dst, int size);
void m68k_not(m68k_state *s, m68k_ea dst, int size);
void m68k_swap(m68k_state *s, int dreg);
void m68k_ext(m68k_state *s, int dreg, int size);
void m68k_mulu(m68k_state *s, m68k_ea src, int dreg);
void m68k_muls(m68k_state *s, m68k_ea src, int dreg);
void m68k_divu(m68k_state *s, m68k_ea src, int dreg);
void m68k_divs(m68k_state *s, m68k_ea src, int dreg);
void m68k_shift(m68k_state *s, int op, uint32_t count, m68k_ea dst, int size);
void m68k_bitop(m68k_state *s, int op, uint32_t bit, m68k_ea dst);

void m68k_movem_to_mem(m68k_state *s, uint16_t mask, m68k_ea dst, int size);
void m68k_movem_to_regs(m68k_state *s, uint16_t mask, m68k_ea src, int size);
void m68k_lea(m68k_state *s, m68k_ea src, int areg);
void m68k_pea(m68k_state *s, m68k_ea src);
void m68k_link(m68k_state *s, int areg, int32_t disp);
void m68k_unlk(m68k_state *s, int areg);
void m68k_exg(m68k_state *s, int reg1, int reg2, int both_data);
void m68k_trap(m68k_state *s, unsigned vector);
void m68k_rte(m68k_state *s);
void m68k_rtr(m68k_state *s);
void m68k_stop(m68k_state *s, uint32_t value);
void m68k_interrupt(m68k_state *s);
void m68k_exception(m68k_state *s, unsigned vector);

#ifdef __cplusplus
}
#endif
#endif
