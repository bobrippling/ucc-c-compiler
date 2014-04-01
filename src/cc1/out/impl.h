#ifndef IMPL_H
#define IMPL_H

void impl_store(out_ctx *, out_val *dest, out_val *val);
out_val *impl_load(out_val *from, const struct vreg *reg);
out_val *impl_load_iv(out_val *from);
out_val *impl_load_fp(out_val *from);

out_val *impl_reg_cp(out_ctx *octx, out_val *from, const struct vreg *r);
void impl_reg_swp(out_val *a, out_val *b);

out_val *impl_op(out_ctx *octx, enum op_type, out_val *l, out_val *r);
out_val *impl_op_unary(out_ctx *octx, enum op_type, out_val *);

out_val *impl_deref(out_ctx *octx, out_val *vp);

void impl_branch(out_val *, out_blk *bt, out_blk *bf);

out_val *impl_i2f(out_ctx *octx, out_val *, type *t_i, type *t_f);
out_val *impl_f2i(out_ctx *octx, out_val *, type *t_f, type *t_i);
out_val *impl_f2f(out_ctx *octx, out_val *, type *from, type *to);

out_val *impl_cast_load(
		out_ctx *octx, out_val *vp,
		type *small, type *big,
		int is_signed);

void impl_change_type(type *t);

out_val *impl_call(
		out_ctx *octx,
		out_val *fn, out_val **args,
		type *fnty);

void impl_return(out_ctx *, out_val *, type *);

void impl_func_prologue_save_fp(void);
void impl_func_prologue_save_call_regs(
		type *rf, unsigned nargs,
		int arg_offsets[/*nargs*/]);

void impl_func_prologue_save_variadic(type *rf);
void impl_func_epilogue(out_ctx *, type *);

void impl_undefined(out_ctx *octx);
void impl_set_overflow(void);
void impl_set_nan(type *);

/* scratch register indexing */
int  impl_reg_to_scratch(const struct vreg *);
void impl_scratch_to_reg(int scratch, struct vreg *);

/* callee save register bools */
int impl_reg_is_callee_save(const struct vreg *r, type *fr);

void impl_comment(enum section_type, const char *fmt, va_list l);
void impl_lbl(const char *lbl);

enum flag_cmp op_to_flag(enum op_type op);
const char *flag_cmp_to_str(enum flag_cmp);

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
