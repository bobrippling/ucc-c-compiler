#include <stdlib.h>

#include "expr.h"
#include "type_is.h"
#include "type_nav.h"
#include "cc1.h"
#include "funcargs.h"
#include "mangle.h"

#include "sanitize.h"

static void sanitize_assert(const out_val *cond, out_ctx *octx, const char *desc)
{
	out_blk *land = out_blk_new(octx, "san_end");
	out_blk *blk_undef = out_blk_new(octx, "san_bad");

	out_ctrl_branch(octx,
			cond,
			land,
			blk_undef);

	out_current_blk(octx, blk_undef);
	out_comment(octx, "sanitizer for %s", desc);
	if(cc1_sanitize_handler_fn){
		type *voidty = type_nav_btype(cc1_type_nav, type_void);
		funcargs *args = funcargs_new();
		type *fnty_noptr = type_func_of(voidty, args, NULL);
		type *fnty_ptr = type_ptr_to(fnty_noptr);
		char *mangled = func_mangle(cc1_sanitize_handler_fn, fnty_noptr);

		const out_val *fn = out_new_lbl(octx, fnty_ptr, mangled, OUT_LBL_NOPIC);

		out_val_release(octx, out_call(octx, fn, NULL, fnty_ptr));

		if(mangled != cc1_sanitize_handler_fn)
			free(mangled);
	}
	out_ctrl_end_undefined(octx);

	out_current_blk(octx, land);
}

static void sanitize_assert_order_runtime(
		const out_val *test, enum op_type op, const out_val *against,
		out_ctx *octx)
{
	const out_val *cmp = out_op(octx, op, test, against);

	sanitize_assert(cmp, octx);
}

static void sanitize_assert_order(
		const out_val *test, enum op_type op, long limit,
		type *op_type, out_ctx *octx, const char *desc)
{
	const out_val *index = out_change_type(octx, out_val_retain(test), op_type);
	const out_val *vsz = out_new_l(octx, op_type, limit);

	sanitize_assert_order_runtime(index, op, vsz, octx);
}

static type *uintptr_ty(void)
{
	return type_nav_btype(cc1_type_nav, type_uintptr_t);
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
	const out_val *runtime_idx;

	if(!(cc1_sanitize & CC1_UBSAN))
		return;

	if(type_is_ptr(elhs->tree_type))
		array_decl = expr_to_declref(elhs, NULL), runtime_idx = rhs;
	else if(type_is_ptr(erhs->tree_type))
		array_decl = expr_to_declref(erhs, NULL), runtime_idx = lhs;

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

	/* force unsigned compare, which catches negative indexes */
	sanitize_assert_order(
			runtime_idx, op_lt, sz.bits.num.val.i,
			"bounds");
}

void sanitize_vlacheck(const out_val *vla_sz, out_ctx *octx)
{
	if(!(cc1_sanitize & CC1_UBSAN))
		return;

	sanitize_assert_order(vla_sz, op_gt, 0, uintptr_ty(), octx, "vla");
}

void sanitize_shift(
		expr *elhs, expr *erhs,
		enum op_type op,
		out_ctx *octx,
		const out_val *lhs, const out_val *rhs)
{
	/* rhs must be < bit-size of lhs' type */
	const unsigned max = CHAR_BIT * type_size(elhs->tree_type, NULL);

	if(!(cc1_sanitize & CC1_UBSAN))
		return;

	out_comment(octx, "rhs less than %u", max);
	sanitize_assert_order(rhs, op_lt, max, uintptr_ty(), octx, "shift rhs size");

	/* rhs must not be negative */
	sanitize_assert_order(rhs, op_ge, 0, erhs->tree_type, octx, "shift rhs negative");

	/* if left shift, lhs must not be negative */
	if(op == op_shiftl)
		sanitize_assert_order(lhs, op_ge, 0, elhs->tree_type, octx, "shift lhs negative");
}
