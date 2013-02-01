#ifndef MIPSEL_32_H
#define MIPSEL_32_H

#define MIPS_REG_ZERO 0

/* (values) from expression evaluation and function results */
#define MIPS_REG_V0 2
#define MIPS_REG_V1 3

/* args */
#define MIPS_REG_A0 4
#define MIPS_REG_A1 5
#define MIPS_REG_A2 6
#define MIPS_REG_A3 7

/* (temporaries) Caller saved if needed.
 * Subroutines can use w/out saving.
 * Not preserved across procedure calls */
#define MIPS_REG_TMP_0 8
#define MIPS_REG_TMP_1 9
#define MIPS_REG_TMP_2 10
#define MIPS_REG_TMP_3 11
#define MIPS_REG_TMP_4 12
#define MIPS_REG_TMP_5 13
#define MIPS_REG_TMP_6 14
#define MIPS_REG_TMP_7 15

/* Callee saved.
 * A subroutine using one of these must save original and restore it before exiting.
 * Preserved across procedure calls */
#define MIPS_REG_SAVE_0 17
#define MIPS_REG_SAVE_1 18
#define MIPS_REG_SAVE_2 19
#define MIPS_REG_SAVE_3 20
#define MIPS_REG_SAVE_4 21
#define MIPS_REG_SAVE_5 22
#define MIPS_REG_SAVE_6 23

/* (temporaries) Caller saved if needed.
 * Subroutines can use w/out saving.
 * These are in addition to $t0 - $t7 above.
 * Not preserved across procedure calls. */
#define MIPS_REG_TMP_8  17
#define MIPS_REG_TMP_9  18
#define MIPS_REG_TMP_10 19
#define MIPS_REG_TMP_11 20
#define MIPS_REG_TMP_12 21
#define MIPS_REG_TMP_13 22
#define MIPS_REG_TMP_14 23

#define MIPS_REG_SP 29
#define MIPS_REG_FP 30
#define MIPS_REG_RA 31

#define N_SCRATCH_REGS 15 /* $t0-$t14 */
#define FIRST_SCRATCH_REG MIPS_REG_TMP_0

#define N_CALL_REGS    4
#define REG_RET        MIPS_REG_V0
#define REG_SP         MIPS_REG_SP

#define IMPL_COMMENT "#"

#endif
