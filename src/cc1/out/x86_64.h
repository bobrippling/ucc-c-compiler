#ifndef X86_64_H
#define X86_64_H

void impl_comment(const char *, va_list);

void impl_store(struct vstack *from, struct vstack *to);
void impl_load(struct vstack *from, int reg);

void impl_reg_cp(struct vstack *from, int r);

void impl_op(enum op_type);
void impl_op_unary(enum op_type); /* returns reg that the result is in */
void impl_deref(void);
void impl_normalise(void);

void impl_jmp(void);
void impl_jcond(int true, const char *lbl);

void impl_cast(decl *from, decl *to);

void impl_call(const int nargs, decl *d);
void impl_call_fin(int nargs);

void impl_lbl(const char *);

int  impl_alloc_stack(int sz);
void impl_free_stack( int sz);

void impl_func_prologue(int stack_res, int nargs, int variadic);
void impl_func_epilogue(void);
void impl_pop_func_ret(decl *);

void impl_undefined(void);
int impl_frame_ptr_to_reg(int nframes);

#define N_REGS 4
#define N_CALL_REGS 6

#define REG_A 0
#define REG_B 1
#define REG_C 2
#define REG_D 3
#define REG_LAST REG_D

/* shared between x86 and __asm */
void out_asm(const char *fmt, ...) ucc_printflike(1, 2);
const char *vstack_str(struct vstack *vs);

#endif
