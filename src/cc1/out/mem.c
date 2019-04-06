#include <stddef.h>
#include <assert.h>

#include "../type_nav.h"

#include "out.h"
#include "val.h" /* for .t */

#include "../fopt.h"
#include "../cc1.h"

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

	if(cc1_fopt.verbose_asm)
		out_comment(octx, "generated memcpy of %zu bytes", nbytes);

	if(i == 0){
		out_val_consume(octx, src);
		return dest;
	}

	tptr = type_ptr_to(type_nav_MAX_FOR(cc1_type_nav, nbytes, 0));
	tptr_sz = type_size(tptr, NULL);

	dest = out_change_type(octx, dest, tptr);
	src = out_change_type(octx, src, tptr);

	while(i > 0){
		/* as many copies as we can at size tptr_sz, then half */
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

void out_memset(
		out_ctx *octx,
		const out_val *v_ptr,
		unsigned char byte,
		unsigned long nbytes)
{
	size_t n, rem;
	unsigned i;
	type *tzero = type_nav_MAX_FOR(cc1_type_nav, nbytes, 0);

	type *textra, *textrap;

	assert(byte == 0 && "TODO: non-zero out_memset()");

	if(!tzero)
		tzero = type_nav_btype(cc1_type_nav, type_nchar);

	n   = nbytes / type_size(tzero, NULL);
	rem = nbytes % type_size(tzero, NULL);

	if((textra = rem ? type_nav_MAX_FOR(cc1_type_nav, rem, 0) : NULL))
		textrap = type_ptr_to(textra);

	v_ptr = out_change_type(octx, v_ptr, type_ptr_to(tzero));

#ifdef MEMSET_VERBOSE
	out_comment("memset(%s, %d, %lu), using ptr<%s>, %lu steps",
			e->expr->f_str(),
			e->bits.builtin_memset.ch,
			e->bits.builtin_memset.len,
			type_to_str(tzero), n);
#endif

	for(i = 0; i < n; i++){
		const out_val *v_zero = out_new_zero(octx, tzero);
		const out_val *v_inc;

		/* *p = 0 */
		out_val_retain(octx, v_ptr);
		out_store(octx, v_ptr, v_zero);

		/* p++ (copied pointer) */
		v_inc = out_new_l(octx, type_nav_btype(cc1_type_nav, type_intptr_t), 1);

		v_ptr = out_op(octx, op_plus, v_ptr, v_inc);

		if(rem){
			/* need to zero a little more */
			v_ptr = out_change_type(octx, v_ptr, textrap);
			v_zero = out_new_zero(octx, textra);

			out_val_retain(octx, v_ptr);
			out_store(octx, v_ptr, v_zero);
		}
	}

	out_val_release(octx, v_ptr);
}
