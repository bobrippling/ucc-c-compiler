#ifndef OUT_H
#define OUT_H

#include "../type.h"
#include "../num.h"
#include "../sym.h"
#include "../op.h"

#include "forwards.h"

out_ctx *out_ctx_new(void);
void out_ctx_end(out_ctx *);

size_t out_expr_stack(out_ctx *);
void out_expr_stack_assert(out_ctx *octx, size_t prev);

/* value creation */
out_val *out_new_num(out_ctx *, type *t, const numeric *n)
	ucc_nonnull((1)) ucc_wur;

out_val *out_new_l(out_ctx *, type *, long) ucc_nonnull((1))
	ucc_wur;

out_val *out_new_zero(out_ctx *, type *) ucc_nonnull((1))
	ucc_wur;

out_val *out_new_lbl(out_ctx *, const char *s, int pic)
	ucc_wur;

out_val *out_new_blk_addr(out_ctx *, out_blk *) ucc_wur;

out_val *out_new_noop(out_ctx *) ucc_wur;

out_val *out_new_sym(out_ctx *, sym *) ucc_wur;
out_val *out_new_sym_val(out_ctx *, sym *) ucc_wur;

out_val *out_new_overflow(out_ctx *) ucc_wur;

out_val *out_new_frame_ptr(out_ctx *, int nframes) ucc_wur;
out_val *out_new_reg_save_ptr(out_ctx *) ucc_wur;
out_val *out_new_nan(out_ctx *, type *ty) ucc_wur;

out_val *out_normalise(out_ctx *, out_val *) ucc_wur;


/* by default, all values are temporaries
 * this will prevent them being overwritten */
out_val *out_val_retain(out_ctx *, out_val *);
out_val *out_val_release(out_ctx *, out_val *);
#define out_val_consume(oc, v) out_val_release((oc), (v))


/* value use */
void out_set_bitfield(out_ctx *, out_val *, unsigned off, unsigned nbits);

void out_store(out_ctx *, out_val *dest, out_val *val);

void out_flush_volatile(out_ctx *, out_val *);

/* operators/comparisons */
ucc_wur out_val *out_op(out_ctx *, enum op_type, out_val *lhs, out_val *rhs);
ucc_wur out_val *out_op_unary(out_ctx *, enum op_type, out_val *);

ucc_wur out_val *out_deref(out_ctx *, out_val *) ucc_wur;

ucc_wur out_val *out_cast(out_ctx *, out_val *, type *to, int normalise_bool)
	ucc_nonnull((1)) ucc_wur;

ucc_wur out_val *out_change_type(out_ctx *, out_val *, type *)
	ucc_nonnull((1)) ucc_wur;

/* functions */
ucc_wur out_val *out_call(out_ctx *,
		out_val *fn, out_val **args,
		type *fnty)
		ucc_nonnull((2, 3));


/* control flow */
ucc_wur out_blk *out_blk_new(const char *desc);
void out_current_blk(out_ctx *, out_blk *) ucc_nonnull((1));

void out_ctrl_end_undefined(out_ctx *);
void out_ctrl_end_ret(out_ctx *, out_val *, type *) ucc_nonnull((1));

void out_ctrl_transfer(out_ctx *, out_blk *to, out_val *phi_arg /* optional */);

/* goto *<exp> */
void out_ctrl_transfer_exp(out_ctx *, out_val *addr);

void out_ctrl_branch(
		out_ctx *octx,
		out_val *cond,
		out_blk *if_true, out_blk *if_false);

/* maybe ret null */
ucc_wur out_val *out_ctrl_merge(out_ctx *, out_blk *, out_blk *);


/* function setup */
void out_func_prologue(
		out_ctx *, const char *sp,
		type *rf,
		int stack_res, int nargs, int variadic,
		int arg_offsets[], int *local_offset);

void out_func_epilogue(out_ctx *, type *, char *end_dbg_lbl);

/* commenting */
void out_comment(const char *, ...) ucc_printflike(1, 2);

#endif
