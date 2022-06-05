#ifndef ARMV7_H
#define ARMV7_H

/*
 * eabi:
 * r0-r3 are the argument and scratch registers; r0-r1 are also the result
 * registers
 *
 * r4-r8 are callee-save registers
 *
 * r9 might be a callee-save register or not
 *
 * r10-r11 are callee-save registers
 * r12-r15 are special registers
 */

#define ARM_REG_R0   0
#define ARM_REG_R1   1
#define ARM_REG_R2   2
#define ARM_REG_R3   3
#define ARM_REG_R4   4
#define ARM_REG_R5   5
#define ARM_REG_R6   6
#define ARM_REG_R7   7
#define ARM_REG_R8   8
#define ARM_REG_R9   9
#define ARM_REG_R10 10
#define ARM_REG_R11 11 /* fp */
#define ARM_REG_R12 12 /* ip: inter-proc scratch */
#define ARM_REG_R13 13 /* sp */
#define ARM_REG_R14 14 /* lr */
#define ARM_REG_R15 15 /* pc */

#define N_SCRATCH_REGS_I 10 /* 0-3, 4-8, 12 */
#define N_SCRATCH_REGS_F 0 /*TODO*/

#define REG_SP ARM_REG_R13
#define REG_BP ARM_REG_R11

#define REG_RET_F 0 /*TODO*/
#define REG_RET_I ARM_REG_R0

/* FIXME */
#define REG_RET_F_1 REG_RET_F
#define REG_RET_I_1 REG_RET_I
#define N_CALL_REGS_F 0
/* end FIXME */

#define VAL_STR_SZ 128

#endif