#include <stdlib.h>

#include "expr.h"
#include "type_is.h"
#include "type_nav.h"
#include "cc1.h"
#include "funcargs.h"
#include "mangle.h"

#include "sanitize.h"

static void sanitize_assert(const out_val *cond, out_ctx *octx)
{
	out_blk *land = out_blk_new(octx, "oob_end");
	out_blk *blk_undef = out_blk_new(octx, "oob_bad");

	out_ctrl_branch(octx,
			cond,
			blk_undef,
			land);

	out_current_blk(octx, blk_undef);
	if(cc1_sanitize_handler_fn){
		type *voidty = type_nav_btype(cc1_type_nav, type_void);
		funcargs *args = funcargs_new();
		type *fnty_noptr = type_func_of(voidty, args, NULL);
		type *fnty_ptr = type_ptr_to(fnty_noptr);
		char *mangled = func_mangle(cc1_sanitize_handler_fn, fnty_noptr);

		const out_val *fn = out_new_lbl(octx, fnty_ptr, mangled, 0);

		out_val_release(octx, out_call(octx, fn, NULL, fnty_ptr));

		if(mangled != cc1_sanitize_handler_fn)
			free(mangled);
	}
	out_ctrl_end_undefined(octx);

	out_current_blk(octx, land);
}

static void sanitize_assert_order(
		const out_val *test, enum op_type op, long limit, out_ctx *octx)
{
	/* force unsigned compare, which catches negative indexes */
	type *uintptr_ty = type_nav_btype(cc1_type_nav, type_uintptr_t);

	const out_val *vsz = out_new_l(
			octx,
			uintptr_ty,
			limit);

	const out_val *index = out_change_type(
			octx,
			out_val_retain(octx, test),
			uintptr_ty);

	const out_val *cmp = out_op(octx, op, vsz, index);

	sanitize_assert(cmp, octx);
}

void sanitize_boundscheck(
		expr *elhs, expr *erhs,
		out_ctx *octx,
		const out_val *lhs, const out_val *rhs)
{
	decl *array_decl = NULL;
	type *array_ty;
	expr *expr_sz;
	consty sz;
	const out_val *val;

	if(!(cc1_sanitize & CC1_UBSAN))
		return;

	if(type_is_ptr(elhs->tree_type))
		array_decl = expr_to_declref(elhs, NULL), val = rhs;
	else if(type_is_ptr(erhs->tree_type))
		array_decl = expr_to_declref(erhs, NULL), val = lhs;

	if(!array_decl)
		return;

	if(!(array_ty = type_is(array_decl->ref, type_array)))
		return;

	expr_sz = array_ty->bits.array.size;
	if(!expr_sz)
		return;
	const_fold(expr_sz, &sz);

	if(sz.type != CONST_NUM)
		return;

	if(!K_INTEGRAL(sz.bits.num))
		return;

	sanitize_assert_order(val, op_lt, sz.bits.num.val.i, octx);
}

void sanitize_vlacheck(const out_val *vla_sz, out_ctx *octx)
{
	if(!(cc1_sanitize & CC1_UBSAN))
		return;

	sanitize_assert_order(vla_sz, op_gt, 0, octx);
}
