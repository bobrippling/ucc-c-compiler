#include <stddef.h>

#include "../type_nav.h"

#include "out.h"
#include "val.h" /* for .t */

static void out_memcpy_single(
		out_ctx *octx,
		const out_val **dst, const out_val **src)
{
	type *t1 = type_nav_btype(cc1_type_nav, type_intptr_t);

	out_val_retain(octx, *dst);
	out_val_retain(octx, *src);
	out_store(octx, *dst, out_deref(octx, *src));

	*dst = out_op(octx, op_plus, *dst, out_new_l(octx, t1, 1));
	*src = out_op(octx, op_plus, *src, out_new_l(octx, t1, 1));
}

const out_val *out_memcpy(
		out_ctx *octx,
		const out_val *dest, const out_val *src,
		unsigned long nbytes)
{
#ifdef BUILTIN_USE_LIBC
	/* TODO - also with memset */
	funcargs *fargs = funcargs_new();

	dynarray_add(&fargs->arglist, decl_new_tref(NULL, type_cached_VOID_PTR()));
	dynarray_add(&fargs->arglist, decl_new_tref(NULL, type_cached_VOID_PTR()));
	dynarray_add(&fargs->arglist, decl_new_tref(NULL, type_cached_INTPTR_T()));

	type *ctype = type_new_func(
			e->tree_type, fargs);

	out_push_lbl("memcpy", 0);
	out_push_l(type_cached_INTPTR_T(), e->bits.num.val);
	lea_expr(e->rhs, stab);
	lea_expr(e->lhs, stab);
	out_call(3, e->tree_type, ctype);
#else
	size_t i = nbytes;
	type *tptr;
	unsigned tptr_sz;

	if(i > 0){
		tptr = type_ptr_to(type_nav_MAX_FOR(cc1_type_nav, nbytes, 0));
		tptr_sz = type_size(tptr, NULL);
	}

	while(i > 0){
		/* as many copies as we can */
		dest = out_change_type(octx, dest, tptr);
		src = out_change_type(octx, src, tptr);

		while(i >= tptr_sz){
			i -= tptr_sz;
			out_memcpy_single(octx, &dest, &src);
		}

		if(i > 0){
			tptr_sz /= 2;
			tptr = type_ptr_to(type_nav_MAX_FOR(cc1_type_nav, tptr_sz, 0));
		}
	}

	out_val_release(octx, src);
	return out_op(
			octx, op_minus,
			dest, out_new_l(octx, dest->t, nbytes));
#endif
}
