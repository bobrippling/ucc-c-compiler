#include <stddef.h>

#include "expr.h"
#include "type_is.h"
#include "type_nav.h"

#include "sanitize.h"

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

	{
		/* force unsigned compare, which catches negative indexes */
		type *uintptr_ty = type_nav_btype(cc1_type_nav, type_uintptr_t);

		const out_val *vsz = out_new_l(
				octx,
				uintptr_ty,
				sz.bits.num.val.i);

		const out_val *index = out_change_type(
				octx,
				out_val_retain(octx, val),
				uintptr_ty);

		const out_val *cmp = out_op(octx, op_ge, index, vsz);

		out_blk *land = out_blk_new(octx, "oob_end");
		out_blk *blk_undef = out_blk_new(octx, "oob_bad");

		out_ctrl_branch(octx,
				cmp,
				blk_undef,
				land);

		out_current_blk(octx, blk_undef);
		out_ctrl_end_undefined(octx);

		out_current_blk(octx, land);
	}
}
