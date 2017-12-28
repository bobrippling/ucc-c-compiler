#ifndef CHIP8_H
#define CHIP8_H

#define CHIP8_REG_V0  0   /* u8 */
#define CHIP8_REG_V1  1   /* u8 */
#define CHIP8_REG_V2  2   /* u8 */
#define CHIP8_REG_V3  3   /* u8 */
#define CHIP8_REG_V4  4   /* u8 */
#define CHIP8_REG_V5  5   /* u8 */
#define CHIP8_REG_V6  6   /* u8 */
#define CHIP8_REG_V7  7   /* u8 */
#define CHIP8_REG_V8  8   /* u8 */
#define CHIP8_REG_V9  9   /* u8 */
#define CHIP8_REG_V10 10  /* u8 */
#define CHIP8_REG_V11 11  /* u8 */
#define CHIP8_REG_V12 12  /* u8 */
#define CHIP8_REG_V13 13  /* u8 */
#define CHIP8_REG_V14 14  /* u8 */
#define CHIP8_REG_V15 15  /* u8 */
#define CHIP8_REG_V16 16  /* u8 */

#define CHIP8_REG_INDEX 17 /* u16 */
#define CHIP8_REG_PC    18 /* u16 */
#define CHIP8_REG_SP    19 /* u16 */
#define CHIP8_REG_DT    20 /* u16 */

#define N_SCRATCH_REGS_I  16
#define N_CALL_REGS_I     0
#define N_SCRATCH_REGS_F  0 /* xmm0-xmm7 */
#define N_CALL_REGS_F     0 /* xmm0-xmm7 */
#define REG_RET_I         CHIP8_REG_V0
#define REG_RET_F         CHIP8_REG_V0
#define REG_SP            CHIP8_REG_SP
#define REG_BP            CHIP8_REG_V15

#define VAL_STR_SZ 128

#endif
