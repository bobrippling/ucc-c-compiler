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

#define N_SCRATCH_REGS    14 /* ax-dx, di-si, r8-r15 */
#define N_CALL_REGS       6
#define REG_RET           X86_64_REG_RAX
#define REG_SP            X86_64_REG_RSP

#define IMPL_COMMENT "//"

#endif
