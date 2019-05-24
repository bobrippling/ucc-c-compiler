#ifndef X86_64_H
#define X86_64_H

#define X86_64_REG_RAX 0
#define X86_64_REG_RBX 1
#define X86_64_REG_RCX 2
#define X86_64_REG_RDX 3

#define X86_64_REG_RDI 4
#define X86_64_REG_RSI 5

#define X86_64_REG_RBP 6
#define X86_64_REG_RSP 7

/* ensure these (not available on i386) come last,
 * for register indexing reasons */
#define X86_64_REG_R8  8
#define X86_64_REG_R9  9
#define X86_64_REG_R10 10
#define X86_64_REG_R11 11
#define X86_64_REG_R12 12
#define X86_64_REG_R13 13
#define X86_64_REG_R14 14
#define X86_64_REG_R15 15

#define X86_64_REG_XMM0 0
#define X86_64_REG_XMM1 1
#define X86_64_REG_XMM2 2
#define X86_64_REG_XMM3 3
#define X86_64_REG_XMM4 4
#define X86_64_REG_XMM5 5
#define X86_64_REG_XMM6 6
#define X86_64_REG_XMM7 7

#define REG_RET_I_1       X86_64_REG_RAX
#define REG_RET_I_2       X86_64_REG_RDX
#define REG_RET_F_1       X86_64_REG_XMM0
#define REG_RET_F_2       X86_64_REG_XMM1
#define REG_SP            X86_64_REG_RSP
#define REG_BP            X86_64_REG_RBP

#define VAL_STR_SZ 128

#endif
