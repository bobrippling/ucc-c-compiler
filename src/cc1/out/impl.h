#ifndef IMPL_H
#define IMPL_H

void impl_store(struct vstack *from, struct vstack *to);
void impl_load(struct vstack *from, int reg);
void impl_lea( struct vstack *from, int reg);

void impl_reg_cp(struct vstack *from, int r);
void impl_reg_swp(struct vstack *a, struct vstack *b);

void impl_op(enum op_type);
void impl_op_unary(enum op_type); /* returns reg that the result is in */
void impl_deref_reg(void);
void impl_normalise(void);

void impl_jmp_lbl(const char *lbl);
void impl_jmp_reg(int r);
void impl_jcond(int true, const char *lbl);

void impl_cast(type_ref *from, type_ref *to);

void impl_call(const int nargs, type_ref *r_ret, type_ref *r_func);
void impl_pop_func_ret(type_ref *r);

int  impl_alloc_stack(int sz);

void impl_func_prologue(int stack_res, int nargs, int variadic);
void impl_func_epilogue(void);

void impl_undefined(void);
int  impl_frame_ptr_to_reg(int nframes);

enum p_opts
{
	P_NO_INDENT = 1 << 0,
	P_NO_NL     = 1 << 1
};

void out_asm( const char *fmt, ...) ucc_printflike(1, 2);
void out_asm2(enum p_opts opts, const char *fmt, ...) ucc_printflike(2, 3);

void out_load(struct vstack *from, int reg);

void impl_comment(const char *fmt, va_list l);
void impl_lbl(const char *lbl);

enum flag_cmp op_to_flag(enum op_type op);

/* can't do this for gen_deps.sh */
#ifdef CC1_IMPL_FNAME
#  include CC1_IMPL_FNAME
#else
#  warning "no impl defined"
#endif

#endif
