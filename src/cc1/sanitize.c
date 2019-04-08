#include <stdlib.h>

#include "../util/limits.h"

#include "expr.h"
#include "type_is.h"
#include "type_nav.h"
#include "sanitize_opt.h"
#include "funcargs.h"
#include "mangle.h"
#include "out/ctrl.h"
#include "vla.h"

#include "sanitize.h"

void sanitize_fail(out_ctx *octx, const char *desc)
{
	out_comment(octx, "sanitizer for %s", desc);
	if(cc1_sanitize_handler_fn){
		type *voidty = type_nav_btype(cc1_type_nav, type_void);
		funcargs *args = funcargs_new();
		type *fnty_noptr = type_func_of(voidty, args, NULL);
		type *fnty_ptr = type_ptr_to(fnty_noptr);
		char *mangled = func_mangle(cc1_sanitize_handler_fn, fnty_noptr);

		const out_val *fn = out_new_lbl(octx, fnty_ptr, mangled, /* could be in another module */OUT_LBL_PIC);

		out_val_release(octx, out_call(octx, fn, NULL, fnty_ptr));

		if(mangled != cc1_sanitize_handler_fn)
			free(mangled);
	}
	out_ctrl_end_undefined(octx);
}

static void sanitize_assert(const out_val *cond, out_ctx *octx, const char *desc)
{
	out_blk *land = out_blk_new(octx, "san_end");
	out_blk *blk_undef = out_blk_new(octx, "san_bad");

	out_ctrl_branch(octx,
			cond,
			land,
			blk_undef);

	out_current_blk(octx, blk_undef);
	sanitize_fail(octx, desc);

	out_current_blk(octx, land);
}

static void sanitize_assert_order2(
		const out_val *test, enum op_type op, const out_val *against,
		out_ctx *octx, const char *desc)
{
	const out_val *cmp;

	cmp = out_op(octx, op, test, against);

	sanitize_assert(cmp, octx, desc);
}

static void sanitize_assert_order(
		const out_val *test, enum op_type op, long limit,
		type *op_type, out_ctx *octx, const char *desc)
{
	const out_val *vlimit = out_new_l(
			octx,
			op_type,
			limit);

	const out_val *lengthened_test = out_change_type(
			octx,
			out_val_retain(octx, test),
			op_type);

	sanitize_assert_order2(lengthened_test, op, vlimit, octx, desc);
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
	const out_val *val;

	if(!(cc1_sanitize & SAN_BOUNDS))
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

	/* note that the comparison (op_le) allows one-past-the-end,
	 * as we don't know at this point if we're addressing or indexing */
	switch(sz.type){
		case CONST_NUM:
			if(!K_INTEGRAL(sz.bits.num))
				return;

			/* force unsigned compare, which catches negative indexes */
			sanitize_assert_order(val, op_le, sz.bits.num.val.i, uintptr_ty(), octx, "bounds");
			break;

		case CONST_NO:
		{
			/* vla */
			const out_val *bytesize, *byteindex, *tsize;
			type *tnext = type_is_array(array_decl->ref);

			if(type_is_variably_modified(tnext))
				tsize = vla_size(tnext, octx);
			else
				tsize = out_new_l(octx, uintptr_ty(), type_size(tnext, NULL));

			out_val_retain(octx, val);
			byteindex = out_op(octx, op_multiply, val, tsize);

			bytesize = vla_size(array_decl->ref, octx);

			sanitize_assert_order2(byteindex, op_le, bytesize, octx, "vla bounds");
			break;
		}

		default:
			break;
	}
}

void sanitize_vlacheck(const out_val *vla_sz, type *sz_ty, out_ctx *octx)
{
	if(!(cc1_sanitize & SAN_VLA_BOUND))
		return;

	sanitize_assert_order(vla_sz, op_gt, 0, sz_ty, octx, "vla");
}

void sanitize_shift(
		expr *elhs, expr *erhs,
		enum op_type op,
		out_ctx *octx,
		const out_val **const lhs, const out_val **const rhs)
{
	/* rhs must be < bit-size of lhs' type */
	const unsigned max = CHAR_BIT * type_size(elhs->tree_type, NULL);
	out_blk *current;

	if(!(cc1_sanitize & SAN_SHIFT_EXPONENT))
		return;

	current = out_ctx_current_blk(octx);

	/* keep lhs/rhs alive no matter what */
	*lhs = out_val_blockphi_make(octx, *lhs, current);
	*rhs = out_val_blockphi_make(octx, *rhs, current);

	out_comment(octx, "rhs less than %u", max);
	sanitize_assert_order(*rhs, op_lt, max, uintptr_ty(), octx, "shift rhs size");

	/* rhs must not be negative */
	sanitize_assert_order(*rhs, op_ge, 0, erhs->tree_type, octx, "shift rhs negative");

	/* if left shift, lhs must not be negative */
	if(op == op_shiftl)
		sanitize_assert_order(*lhs, op_ge, 0, elhs->tree_type, octx, "shift lhs negative");

	*lhs = out_val_unphi(octx, *lhs);
	*rhs = out_val_unphi(octx, *rhs);
}

void sanitize_divide(const out_val *lhs, const out_val *rhs, type *ty, out_ctx *octx)
{
	const out_val *cmp, *zero;
	const int is_fp = type_is_floating(ty);

	if(is_fp){
		numeric n;
		if(!(cc1_sanitize & SAN_FLOAT_DIVIDE_BY_ZERO))
			return;

		n.suffix = VAL_FLOATING;
		n.val.f = 0;

		zero = out_new_num(octx, ty, &n);
	}else{
		if(!(cc1_sanitize & SAN_INTEGER_DIVIDE_BY_ZERO))
			return;

		zero = out_new_l(octx, ty, 0);
	}

	cmp = out_op(octx, op_ne, out_val_retain(octx, rhs), zero);
	sanitize_assert(cmp, octx, "divide by zero");

	if(is_fp)
		return;

	cmp = out_op(octx, op_or,
			out_op(octx, op_ne, out_val_retain(octx, lhs), out_new_l(octx, ty, UCC_INT_MIN)),
			out_op(octx, op_ne, out_val_retain(octx, rhs), out_new_l(octx, ty, -1)));
	sanitize_assert(cmp, octx, "INT_MIN / -1");
}

void sanitize_nonnull_args(symtable *arg_symtab, out_ctx *octx)
{
	/* by this stage, any nonnull attribute will have been applied to each argument decl type */
	decl **i;

	if(!(cc1_sanitize & SAN_NONNULL_ATTRIBUTE))
		return;

	for(i = symtab_decls(arg_symtab); i && *i; i++){
		decl *d = *i;
		attribute *da = attribute_present(d, attr_nonnull);

		if(!da)
			continue;

		sanitize_assert(
				out_new_sym(octx, d->sym),
				octx,
				"nonnull argument");
	}
}

void sanitize_nonnull(
		const out_val *v, out_ctx *octx, const char *desc)
{
	if(!(cc1_sanitize & SAN_NULL))
		return;
	sanitize_assert(out_val_retain(octx, v), octx, desc);
}

void sanitize_aligned(const out_val *v, out_ctx *octx, type *t)
{
	unsigned mask = type_align(t, NULL) - 1;

	if(!(cc1_sanitize & SAN_ALIGNMENT))
		return;

	sanitize_assert(
			out_op_unary(octx, op_not,
				out_op(octx, op_and,
					out_val_retain(octx, v),
					out_new_l(octx, type_ptr_to(t), mask))),
			octx, "alignment");
}

void sanitize_returns_nonnull(const out_val *v, out_ctx *octx)
{
	if(!(cc1_sanitize & SAN_RETURNS_NONNULL_ATTRIBUTE))
		return;
	sanitize_assert(out_val_retain(octx, v), octx, "returns_nonnull");
}
