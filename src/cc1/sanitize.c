#include <stdlib.h>

#include "../util/dynarray.h"

#include "expr.h"
#include "type_is.h"
#include "type_nav.h"
#include "cc1.h"
#include "funcargs.h"
#include "mangle.h"
#include "vla.h"
#include "out/val.h"

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

void sanitize_nonnull(const out_val *v, out_ctx *octx)
{
	if(!(cc1_sanitize & CC1_UBSAN))
		return;

	out_val_retain(octx, v);

	sanitize_assert(
			out_op(octx, op_ne, v, out_new_l(octx, uintptr_ty(), 0)),
			octx);
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

	out_val_retain(octx, runtime_idx);

	if(sz.type == CONST_NUM){
		if(!K_INTEGRAL(sz.bits.num))
			return;

		out_comment(octx,
				"sanitize array, max=%" NUMERIC_FMT_U,
				sz.bits.num.val.i);

		/* force unsigned compare, which catches negative indexes */
		sanitize_assert_order(
				runtime_idx, op_lt, sz.bits.num.val.i,
				uintptr_ty(), octx,
				"bounds");

	}else{
		const out_val *vla_sz;
		type *array_of = type_next(array_ty);

		if(type_is_vla(array_of, VLA_ANY_DIMENSION)){
			/* can't index-check multidimensional VLAs */
			out_comment(octx, "can't sanitize %s", type_to_str(array_ty));
			return;
		}

		out_comment(octx, "sanitize vla, size:");

		vla_sz = out_op(octx,
				op_divide,
				vla_size(array_ty, octx),
				out_new_l(octx, uintptr_ty(), type_size(array_of, NULL)));

		out_comment(octx, "sanitize vla, check:");

		sanitize_assert_order_runtime(
				out_change_type(octx, runtime_idx, uintptr_ty()),
				op_lt,
				out_change_type(octx, vla_sz, uintptr_ty()),
				octx);
	}
}

void sanitize_vlacheck(const out_val *vla_sz, out_ctx *octx)
{
	if(!(cc1_sanitize & CC1_UBSAN))
		return;

	out_comment(octx, "sanitize vla init (gt 0):");

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

	out_comment(octx, "sanitize shift:");

	out_comment(octx, "rhs less than %u", max);
	sanitize_assert_order(rhs, op_lt, max, uintptr_ty(), octx, "shift rhs size");

	/* rhs must not be negative */
	sanitize_assert_order(rhs, op_ge, 0, erhs->tree_type, octx, "shift rhs negative");

	/* if left shift, lhs must not be negative */
	if(op == op_shiftl)
		sanitize_assert_order(lhs, op_ge, 0, elhs->tree_type, octx, "shift lhs negative");
}

static void sanitize_asan_fail_call(
		const out_val *addr, out_ctx *octx,
		const char *func_spel, const unsigned size)
{
	type *voidty = type_nav_btype(cc1_type_nav, type_void);
	type *intptr_ty = type_nav_btype(cc1_type_nav, type_intptr_t);
	funcargs *args = funcargs_new();
	type *fnty_noptr, *fnty_ptr;
	char *mangled;
	const out_val *fn;
	const out_val *arg_vals[3] = { 0 };
	decl *arg_decls[3] = { 0 };

	arg_decls[0] = decl_new_ty_sp(type_ptr_to(voidty), NULL);
	dynarray_add(&args->arglist, arg_decls[0]);
	arg_vals[0] = addr;
	if(size){
		arg_decls[1] = decl_new_ty_sp(intptr_ty, NULL);
		dynarray_add(&args->arglist, arg_decls[1]);
		arg_vals[1] = out_new_l(octx, intptr_ty, size);
	}

	fnty_noptr = type_func_of(voidty, args, NULL);
	fnty_ptr = type_ptr_to(fnty_noptr);
	mangled = func_mangle(func_spel, fnty_noptr);

	fn = out_new_lbl(octx, fnty_ptr, mangled, 0);

	out_val_release(octx, out_call(octx, fn, arg_vals, fnty_ptr));

	if(mangled != func_spel)
		free(mangled);
}

static void sanitize_asan_fail(const out_val *addr, out_ctx *octx)
{
	/* _Noreturn void __asan_report_load%d(void *); */
	const unsigned sz = type_size(type_is_ptr(addr->t), NULL);
	char fnbuf[32];
	int need_arg = 0;

	switch(sz){
		case 1:
		case 2:
		case 4:
		case 8:
		case 16:
			snprintf(fnbuf, sizeof fnbuf, "__asan_report_load%d", sz);
			break;
		default:
			snprintf(fnbuf, sizeof fnbuf, "__asan_report_load_n");
			need_arg = 1;
			break;
	}

	sanitize_asan_fail_call(addr, octx, fnbuf, need_arg ? sz : 0);

	out_ctrl_end_undefined(octx);
}

static void sanitize_address1(const out_val *addr, out_ctx *octx)
{
	type *const charp_ty = type_ptr_to(type_nav_btype(cc1_type_nav, type_uchar));
	type *const intptr_ty = type_nav_btype(cc1_type_nav, type_intptr_t);

	const out_val *shadow_mem = out_new_l(
			octx, charp_ty, 0x100000000000L);

	const out_val *intaddr = out_change_type(
			octx, out_val_retain(octx, addr), intptr_ty);

	const out_val *shadow_ent;
	const out_val *adjusted_addr;
	const out_val *cmp_ge, *cmp_z;
	const out_val *asan_bad;

	out_blk *blk_good = out_blk_new(octx, "asan_good");
	out_blk *blk_bad = out_blk_new(octx, "asan_bad");

	intaddr = out_op(
			octx,
			op_shiftr,
			intaddr,
			out_new_l(
				octx,
				intptr_ty,
				3));

	shadow_ent = out_cast(
			octx,
			out_deref(
				octx,
				out_change_type(
					octx,
					out_op(
						octx,
						op_plus,
						shadow_mem,
						out_val_retain(octx, intaddr)),
					charp_ty)),
			intptr_ty,
			0);

	adjusted_addr = out_op(
			octx,
			op_plus,
			out_op(
				octx,
				op_and,
				intaddr,
				out_new_l(
					octx,
					intptr_ty,
					7)),
			out_new_l(
				octx,
				intptr_ty,
				3)); // eax

	cmp_z = out_op(
			octx,
			op_eq,
			out_val_retain(octx, shadow_ent),
			out_new_l(
				octx,
				intptr_ty,
				0));

	cmp_ge = out_op(
			octx,
			op_ge,
			adjusted_addr,
			shadow_ent);

	asan_bad = out_op(
			octx,
			op_and,
			cmp_z,
			cmp_ge);

	out_ctrl_branch(octx, asan_bad, blk_bad, blk_good);

	out_current_blk(octx, blk_bad);
	{
		sanitize_asan_fail(out_val_retain(octx, addr), octx);
	}

	out_current_blk(octx, blk_good);
	/* continue onwards... */

#warning todo init
	/*
	__attribute((constructor))
	static void asan_init(void)
	{
		void __asan_init_v4(void);
		__asan_init_v4();
	}
	*/
}

void sanitize_address(const out_val *addr, out_ctx *octx)
{
	if(!(cc1_sanitize & CC1_ASAN))
		return;

	sanitize_address1(addr, octx);
}
