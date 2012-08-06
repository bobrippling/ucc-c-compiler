#ifndef X86_64_H
#define X86_64_H

void impl_comment(const char *, va_list);

void impl_store(int reg, struct vstack *where);
void impl_load(struct vstack *from, int reg);

int  impl_op(enum op_type); /* returns reg that the result is in */
int  impl_op_unary(enum op_type); /* returns reg that the result is in */

void impl_jmp(void);

void impl_make_addr(void);

#define N_REGS 4

#endif
