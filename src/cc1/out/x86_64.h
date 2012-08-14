#ifndef X86_64_H
#define X86_64_H

void impl_comment(const char *, va_list);

void impl_store(struct vstack *from, struct vstack *to);
void impl_load(struct vstack *from, int reg);

void impl_reg_cp(struct vstack *from, int r);

void impl_op(enum op_type);
void impl_op_unary(enum op_type); /* returns reg that the result is in */
void impl_normalise(void);

void impl_jmp(void);
void impl_jtrue(const char *lbl);

void impl_make_addr(void);

void impl_call(int nargs);
void impl_call_fin(int nargs);

#define N_REGS 4
#define N_CALL_REGS 6

#endif
