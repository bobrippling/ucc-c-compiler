#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "../type_nav.h"

#include "out.h"
#include "val.h" /* for .t */

/* for libc */
#include "../funcargs.h"
#include "../cc1.h"
#include "../mangle.h"
#include "../fopt.h"

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

static int should_use_libc(unsigned long nbytes)
{
	if(cc1_fopt.freestanding)
		return 0;

	switch(cc1_mstringop_strategy){
		case STRINGOP_STRATEGY_LIBCALL:
			return 1;
		case STRINGOP_STRATEGY_LOOP:
			return 0;
		case STRINGOP_STRATEGY_THRESHOLD:
			return nbytes >= cc1_mstringop_threshold;
	}
}

static const out_val *emit_libc_call(
		out_ctx *octx,
		const char *func_name,
		const out_val *arg0,
		const out_val *arg1,
		const out_val *arg2)
{
	type *fnret = type_nav_voidptr(cc1_type_nav);
	funcargs *fargs = funcargs_new();
	type *fnty = type_func_of(fnret, fargs, /*scope*/NULL);
	char *mangled = func_mangle(func_name, fnty);
	out_val *fn = out_new_lbl(octx, fnty, mangled, OUT_LBL_PIC);
	const out_val *args[4] = { 0 };
	const out_val *ret;

	args[0] = arg0;
	args[1] = arg1;
	args[2] = arg2;

	ret = out_call(octx, fn, args, type_ptr_to(fnty));

	if(mangled != func_name)
		free(mangled);

	return ret;
}

const out_val *out_memcpy(
		out_ctx *octx,
		const out_val *dest, const out_val *src,
		unsigned long nbytes)
{
	if(nbytes == 0){
		out_val_release(octx, src);
		return dest;
	}

	if(should_use_libc(nbytes)){
		const out_val *nbytes_val = out_new_l(octx, type_nav_btype(cc1_type_nav, type_uintptr_t), nbytes);

		return emit_libc_call(octx, "memcpy", dest, src, nbytes_val);
	}else{
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
	}
}

void out_memset(
		out_ctx *octx,
		const out_val *v_ptr,
		unsigned char byte,
		unsigned long nbytes)
{
	if(nbytes == 0){
		out_val_release(octx, v_ptr);
		return;
	}

	if(should_use_libc(nbytes)){
		const out_val *byte_val = out_new_l(octx, type_nav_btype(cc1_type_nav, type_uchar), byte);
		const out_val *nbytes_val = out_new_l(octx, type_nav_btype(cc1_type_nav, type_uintptr_t), nbytes);

		out_val_consume(octx, emit_libc_call(octx, "memset", v_ptr, byte_val, nbytes_val));
	}else{
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
}
