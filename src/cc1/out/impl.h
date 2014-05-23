#ifndef IMPL_H
#define IMPL_H

#include "../../util/compiler.h"

void impl_store(out_ctx *, const out_val *dest, const out_val *val);
ucc_wur const out_val *impl_load(out_ctx *octx, const out_val *from, const struct vreg *reg);
ucc_wur const out_val *impl_load_iv(const out_val *from);
ucc_wur const out_val *impl_load_fp(const out_val *from);

void impl_reg_cp_no_off(out_ctx *octx, const out_val *from, const struct vreg *to_reg);
void impl_reg_swp(const out_val *a, const out_val *b);

ucc_wur const out_val *impl_op(out_ctx *octx, enum op_type, const out_val *l, const out_val *r);
ucc_wur const out_val *impl_op_unary(out_ctx *octx, enum op_type, const out_val *);

ucc_wur const out_val *impl_deref(
		out_ctx *octx, const out_val *vp,
		/*maybe null:*/const struct vreg *reg);

void impl_branch(out_ctx *, const out_val *, out_blk *bt, out_blk *bf);

ucc_wur const out_val *impl_i2f(out_ctx *octx, const out_val *, type *t_i, type *t_f);
ucc_wur const out_val *impl_f2i(out_ctx *octx, const out_val *, type *t_f, type *t_i);
ucc_wur const out_val *impl_f2f(out_ctx *octx, const out_val *, type *from, type *to);

ucc_wur const out_val *impl_cast_load(
		out_ctx *octx, const out_val *vp,
		type *small, type *big,
		int is_signed);

void impl_change_type(type *t);

ucc_wur const out_val *impl_call(
		out_ctx *octx,
		const out_val *fn, const out_val **args,
		type *fnty);

void impl_to_retreg(out_ctx *, const out_val *, type *retty);

void impl_func_prologue_save_fp(out_ctx *octx);
void impl_func_prologue_save_call_regs(
		out_ctx *,
		type *rf, unsigned nargs,
		int arg_offsets[/*nargs*/]);

void impl_func_prologue_save_variadic(out_ctx *octx, type *rf);
void impl_func_epilogue(out_ctx *, type *);

void impl_undefined(out_ctx *octx);
void impl_set_overflow(void);
void impl_set_nan(type *);

/* scratch register indexing */
int  impl_reg_to_scratch(const struct vreg *);
void impl_scratch_to_reg(int scratch, struct vreg *);
int impl_reg_frame_const(const struct vreg *);
int impl_reg_savable(const struct vreg *);

/* callee save register bools */
int impl_reg_is_callee_save(const struct vreg *r, type *fr);

void impl_comment(out_ctx *, const char *fmt, va_list l);

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
