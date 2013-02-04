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

#define X86_64_REG_RBP 8
#define X86_64_REG_RSP 9

#define N_SCRATCH_REGS    4
#define FIRST_SCRATCH_REG X86_64_REG_RAX
#define N_CALL_REGS       6
#define REG_RET           X86_64_REG_RAX
#define REG_SP            X86_64_REG_RSP

#define IMPL_COMMENT "//"

#endif
