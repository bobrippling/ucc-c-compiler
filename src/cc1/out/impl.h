#ifndef IMPL_H
#define IMPL_H

void impl_store(struct vstack *from, struct vstack *to);
void impl_load(struct vstack *from, const struct vreg *reg);
void impl_load_iv(struct vstack *from);
void impl_load_fp(struct vstack *from);

void impl_reg_cp(struct vstack *from, const struct vreg *r);
void impl_reg_swp(struct vstack *a, struct vstack *b);

void impl_op(enum op_type);
void impl_op_unary(enum op_type);
void impl_deref(
		struct vstack *vp,
		const struct vreg *to,
		type *tpointed_to);

void impl_jmp(void);
void impl_jcond(int true, const char *lbl);

void impl_i2f(struct vstack *, type *t_i, type *t_f);
void impl_f2i(struct vstack *, type *t_f, type *t_i);
void impl_f2f(struct vstack *, type *from, type *to);
void impl_cast_load(
		struct vstack *vp,
		type *small, type *big,
		int is_signed);

void impl_change_type(type *t);

void impl_call(const int nargs, type *r_ret, type *r_func);
void impl_pop_func_ret(type *r);

void impl_func_prologue_save_fp(void);
void impl_func_prologue_save_call_regs(
		type *rf, unsigned nargs,
		int arg_offsets[/*nargs*/]);

void impl_func_prologue_save_variadic(type *rf);
void impl_func_epilogue(type *);

void impl_undefined(void);
void impl_set_overflow(void);
void impl_set_nan(type *);

/* scratch register indexing */
int  impl_reg_to_scratch(const struct vreg *);
void impl_scratch_to_reg(int scratch, struct vreg *);

/* callee save register bools */
int impl_reg_is_callee_save(const struct vreg *r, type *fr);

void impl_comment(enum section_type, const char *fmt, va_list l);
void impl_lbl(const char *lbl);

/* implicitly takes vtop as a pointer to the memory */
void impl_overlay_mem2regs(
		unsigned memsz, unsigned nregs,
		struct vreg regs[]);

/* same - implicit vtop */
void impl_overlay_regs2mem(
		unsigned memsz, unsigned nregs,
		struct vreg regs[]);

enum flag_cmp op_to_flag(enum op_type op);

/* can't do this for gen_deps.sh */
#ifdef CC1_IMPL_FNAME
#  include CC1_IMPL_FNAME
#else
#  warning "no impl defined"
#endif

#ifndef MIN
#  define MIN(x, y) ((x) < (y) ? (x) : (y))
#  define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#endif
