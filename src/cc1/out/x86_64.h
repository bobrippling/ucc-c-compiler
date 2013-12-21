#ifndef X86_64_H
#define X86_64_H

#define X86_64_REG_RAX 0
#define X86_64_REG_RBX 1
#define X86_64_REG_RCX 2
#define X86_64_REG_RDX 3

#define X86_64_REG_RDI 4
#define X86_64_REG_RSI 5

#define X86_64_REG_R8  6
#define X86_64_REG_R9  7
#define X86_64_REG_R10 8
#define X86_64_REG_R11 9
#define X86_64_REG_R12 10
#define X86_64_REG_R13 11
#define X86_64_REG_R14 12
#define X86_64_REG_R15 13

#define X86_64_REG_RBP 14
#define X86_64_REG_RSP 15

#define X86_64_REG_XMM0 0
#define X86_64_REG_XMM1 1
#define X86_64_REG_XMM2 2
#define X86_64_REG_XMM3 3
#define X86_64_REG_XMM4 4
#define X86_64_REG_XMM5 5
#define X86_64_REG_XMM6 6
#define X86_64_REG_XMM7 7

#define N_SCRATCH_REGS_I  14 /* ax-dx, di-si, r8-r15 */
#define N_CALL_REGS_I     6
#define N_SCRATCH_REGS_F  8 /* xmm0-xmm7 */
#define N_CALL_REGS_F     8 /* xmm0-xmm7 */
#define REG_RET_I         X86_64_REG_RAX
#define REG_RET_F         X86_64_REG_XMM0
#define REG_SP            X86_64_REG_RSP
#define REG_BP            X86_64_REG_RBP

#endif
