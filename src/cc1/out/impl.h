#ifndef IMPL_H
#define IMPL_H

void impl_store(struct vstack *from, struct vstack *to);
void impl_load(struct vstack *from, int reg);
void impl_lea( struct vstack *from, int reg);
void impl_load_iv(struct vstack *from);

void impl_reg_cp(struct vstack *from, int r);
void impl_reg_swp(struct vstack *a, struct vstack *b);

void impl_op(enum op_type);
void impl_op_unary(enum op_type); /* returns reg that the result is in */
void impl_deref_reg(void);

void impl_jmp(void);
void impl_jcond(int true, const char *lbl);

void impl_cast_load(struct vstack *vp, type_ref *small, type_ref *big, int is_signed);

void impl_call(const int nargs, type_ref *r_ret, type_ref *r_func);
void impl_pop_func_ret(type_ref *r);

void impl_func_prologue_save_fp(void);
void impl_func_prologue_save_call_regs(int nargs);
int  impl_func_prologue_save_variadic(int nargs);
void impl_func_epilogue(void);

void impl_undefined(void);
void impl_set_overflow(void);
int  impl_frame_ptr_to_reg(int nframes);

/* scratch register indexing */
int impl_reg_to_scratch(int);
int impl_scratch_to_reg(int);

void impl_comment(const char *fmt, va_list l);
void impl_lbl(const char *lbl);

enum flag_cmp op_to_flag(enum op_type op);

/* can't do this for gen_deps.sh */
#ifdef CC1_IMPL_FNAME
#  include CC1_IMPL_FNAME
#else
#  warning "no impl defined"
#endif

#ifndef MIN
#  define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#endif
