#ifndef OUT_H
#define OUT_H

#include "../type.h"
#include "../num.h"
#include "../sym.h"
#include "../op.h"
#include "../sanitize_opt.h"

#include "forwards.h"

out_ctx *out_ctx_new(void);
void out_ctx_end(out_ctx *);
void out_perfunc_init(out_ctx *, decl *fndecl, const char *sp);
void out_perfunc_teardown(out_ctx *);

void **out_user_ctx(out_ctx *);

int out_dump_retained(out_ctx *octx, const char *desc);

/* value creation */
out_val *out_new_num(out_ctx *, type *t, const numeric *n)
	ucc_nonnull((1)) ucc_wur;

out_val *out_new_l(out_ctx *, type *, long) ucc_nonnull((1))
	ucc_wur;

out_val *out_new_zero(out_ctx *, type *) ucc_nonnull((1))
	ucc_wur;

/* pic: generate position independent accesses to the label
 * local_sym: symbol/label is present in the current C.U.
 *            pass false if unsure
 */
out_val *out_new_lbl(out_ctx *, type *, const char *s, enum out_pic_type)
	ucc_wur;

out_val *out_new_blk_addr(out_ctx *, out_blk *) ucc_wur;

out_val *out_new_noop(out_ctx *) ucc_wur;

const out_val *out_new_sym(out_ctx *, sym *) ucc_wur;
const out_val *out_new_sym_val(out_ctx *, sym *) ucc_wur;

/* modifies expr/val and returns the overflow check expr/val */
const out_val *out_new_overflow(out_ctx *, const out_val **) ucc_wur;

out_val *out_new_frame_ptr(out_ctx *, int nframes) ucc_wur;
const out_val *out_new_return_addr(out_ctx *octx, int nframes) ucc_wur;
out_val *out_new_reg_save_ptr(out_ctx *) ucc_wur;
out_val *out_new_nan(out_ctx *, type *ty) ucc_wur;

const out_val *out_normalise(out_ctx *, const out_val *) ucc_wur;


/* by default, all values are temporaries
 * this will prevent them being overwritten */
const out_val *out_val_retain(out_ctx *, const out_val *);
const out_val *out_val_release(out_ctx *, const out_val *);
#define out_val_consume(oc, v) out_val_release((oc), (v))


/* value use */
const out_val *out_set_bitfield(
		out_ctx *,
		const out_val *,
		unsigned off,
		unsigned nbits,
		type *master_ty)
	ucc_wur;

void out_store(out_ctx *, const out_val *dest, const out_val *val);

void out_flush_volatile(out_ctx *, const out_val *);

ucc_wur const out_val *out_annotate_likely(
		out_ctx *, const out_val *, int unlikely);

/* operators/comparisons */
ucc_wur const out_val *out_op(out_ctx *, enum op_type, const out_val *lhs, const out_val *rhs);
ucc_wur const out_val *out_op_unary(out_ctx *, enum op_type, const out_val *);

ucc_wur const out_val *out_memcpy(
		out_ctx *octx,
		const out_val *dest, const out_val *src,
		unsigned long bytes);

void out_memset(
		out_ctx *octx,
		const out_val *dest,
		unsigned char byte,
		unsigned long nbytes);

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
ucc_wur out_blk *out_ctx_current_blk(out_ctx *);
ucc_wur int out_ctx_current_blk_is_empty(out_ctx *);

out_blk *out_blk_entry(out_ctx *);
out_blk *out_blk_postprologue(out_ctx *);

int out_sanitize_enabled(out_ctx *, enum san_opts opt);

void out_ctrl_end_undefined(out_ctx *);
void out_ctrl_end_ret(out_ctx *, const out_val *, type *) ucc_nonnull((1));
void out_ctrl_debugtrap(out_ctx *);

/* Will the value be used only in *mergee? (/ not immediately afterwards, in
 * the current block)
 * If so, stash_phi_value, otherwise, keep live
 * (this allows us to prevent later-spills of the value
 * not being reflected in sub-blocks of *mergee)
 */
void out_ctrl_transfer(out_ctx *octx, out_blk *to,
		/* optional: */
		const out_val *phi, out_blk **mergee,
		int stash_phi_value);

void out_ctrl_transfer_make_current(out_ctx *octx, out_blk *to);

/* goto *<exp> */
void out_ctrl_transfer_exp(out_ctx *, const out_val *addr);

void out_ctrl_branch(
		out_ctx *octx,
		const out_val *cond,
		out_blk *if_true, out_blk *if_false);

void out_blk_mustgen(out_ctx *octx, out_blk *blk, char *force_lbl);

/* maybe ret null */
ucc_wur const out_val *out_ctrl_merge(out_ctx *, out_blk *, out_blk *);

ucc_wur const out_val *out_ctrl_merge_n(out_ctx *, out_blk **rets);

/* function setup */
void out_func_prologue(
		out_ctx *,
		int nargs, int variadic, int stack_protector,
		const out_val *argvals[]);

struct section;
void out_func_epilogue(
	out_ctx *,
	type *,
	const where *func_begin,
	char *end_dbg_lbl,
	const struct section *section);


/* returns a pointer to allocated storage: */
const out_val *out_alloca_push(out_ctx *, const out_val *sz, unsigned align);
/* alloca_restore restores the stack for scope-leave.
 * alloca_pop restores the stack and cleans up internal vla state */
void out_alloca_restore(out_ctx *octx, const out_val *ptr);
void out_alloca_pop(out_ctx *octx);

const out_val *out_aalloc(out_ctx *, unsigned sz, unsigned align, type *, long *offset);
const out_val *out_aalloct(out_ctx *, type *);
void out_adealloc(out_ctx *, const out_val **);

void out_force_read(out_ctx *octx, type *, const out_val *);

const char *out_get_lbl(const out_val *) ucc_nonnull();
int out_is_nonconst_temporary(const out_val *) ucc_nonnull();
unsigned out_current_stack(out_ctx *); /* used in inlining */

/* commenting */
void out_comment(out_ctx *, const char *, ...) ucc_printflike(2, 3);
const char *out_val_str(const out_val *, int deref);

void test_out_out(void);

#endif
