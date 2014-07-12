#ifndef OUT_H
#define OUT_H

#include "../type.h"
#include "../num.h"
#include "../sym.h"
#include "../op.h"

#include "forwards.h"

out_ctx *out_ctx_new(void);
void out_ctx_end(out_ctx *);
void out_ctx_wipe(out_ctx *);

size_t out_expr_stack(out_ctx *);
void out_dump_retained(out_ctx *octx, const char *desc);

/* value creation */
out_val *out_new_num(out_ctx *, type *t, const numeric *n)
	ucc_nonnull((1)) ucc_wur;

out_val *out_new_l(out_ctx *, type *, long) ucc_nonnull((1))
	ucc_wur;

out_val *out_new_zero(out_ctx *, type *) ucc_nonnull((1))
	ucc_wur;

out_val *out_new_lbl(out_ctx *, type *, const char *s, int pic)
	ucc_wur;

out_val *out_new_blk_addr(out_ctx *, out_blk *) ucc_wur;

out_val *out_new_noop(out_ctx *) ucc_wur;

const out_val *out_new_sym(out_ctx *, sym *) ucc_wur;
const out_val *out_new_sym_val(out_ctx *, sym *) ucc_wur;

/* modifies expr/val and returns the overflow check expr/val */
const out_val *out_new_overflow(out_ctx *, const out_val **) ucc_wur;

out_val *out_new_frame_ptr(out_ctx *, int nframes) ucc_wur;
out_val *out_new_reg_save_ptr(out_ctx *) ucc_wur;
out_val *out_new_nan(out_ctx *, type *ty) ucc_wur;

const out_val *out_normalise(out_ctx *, const out_val *) ucc_wur;


/* by default, all values are temporaries
 * this will prevent them being overwritten */
const out_val *out_val_retain(out_ctx *, const out_val *);
const out_val *out_val_release(out_ctx *, const out_val *);
#define out_val_consume(oc, v) out_val_release((oc), (v))


/* value use */
const out_val *out_set_bitfield(out_ctx *, const out_val *, unsigned off, unsigned nbits)
	ucc_wur;

void out_store(out_ctx *, const out_val *dest, const out_val *val);

void out_flush_volatile(out_ctx *, const out_val *);

ucc_wur const out_val *out_annotate_likely(
		out_ctx *, const out_val *, int unlikely);

/* operators/comparisons */
ucc_wur const out_val *out_op(out_ctx *, enum op_type, const out_val *lhs, const out_val *rhs);
ucc_wur const out_val *out_op_unary(out_ctx *, enum op_type, const out_val *);

ucc_wur const out_val *out_deref(out_ctx *, const out_val *) ucc_wur;

ucc_wur const out_val *out_cast(out_ctx *, const out_val *, type *to, int normalise_bool)
	ucc_nonnull((1)) ucc_wur;

ucc_wur const out_val *out_change_type(out_ctx *, const out_val *, type *)
	ucc_nonnull((1)) ucc_wur;

/* functions */
ucc_wur const out_val *out_call(out_ctx *,
		const out_val *fn, const out_val **args,
		type *fnty)
		ucc_nonnull((1, 2, 4));


/* control flow */
ucc_wur out_blk *out_blk_new(out_ctx *, const char *desc);
void out_current_blk(out_ctx *, out_blk *) ucc_nonnull((1));

void out_ctrl_end_undefined(out_ctx *);
void out_ctrl_end_ret(out_ctx *, const out_val *, type *) ucc_nonnull((1));

void out_ctrl_transfer(out_ctx *octx, out_blk *to,
		/* optional: */
		const out_val *phi, out_blk **mergee);

void out_ctrl_transfer_make_current(out_ctx *octx, out_blk *to);

/* goto *<exp> */
void out_ctrl_transfer_exp(out_ctx *, const out_val *addr);

void out_ctrl_branch(
		out_ctx *octx,
		const out_val *cond,
		out_blk *if_true, out_blk *if_false);

/* maybe ret null */
ucc_wur const out_val *out_ctrl_merge(out_ctx *, out_blk *, out_blk *);


/* function setup */
void out_func_prologue(
		out_ctx *, const char *sp,
		type *fnty,
		int stack_res, int nargs, int variadic,
		int arg_offsets[], int *local_offset);

void out_func_epilogue(out_ctx *, type *, char *end_dbg_lbl);

/* commenting */
void out_comment(out_ctx *, const char *, ...) ucc_printflike(2, 3);

#endif
