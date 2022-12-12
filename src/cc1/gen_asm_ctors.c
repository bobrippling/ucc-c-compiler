#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "../util/dynarray.h"

#include "decl.h"
#include "const.h"
#include "cc1_target.h"

#include "expr.h"
#include "ops/expr_identifier.h"
#include "ops/expr_val.h"
#include "type_nav.h"
#include "gen_asm.h"
#include "funcargs.h"

#include "out/asm.h"

#include "gen_asm_ctors.h"

static int decl_cmp_ctor_dtor_pri(
		const void *a, const void *b, enum attribute_type attr_ty)
{
	decl *da = *(decl **)a, *db = *(decl **)b;
	attribute *attr_a = attribute_present(da, attr_ty);
	attribute *attr_b = attribute_present(db, attr_ty);

	assert(attr_a && attr_b);

	{
		struct expr *exp_a = attr_a->bits.priority;
		struct expr *exp_b = attr_b->bits.priority;
		integral_t ia, ib;

		switch(!!exp_a + !!exp_b){
			case 0:
				return 0;

			case 1:
				return exp_a ? -1 : 1;

			case 2:
				break;

			default:
				assert(0);
		}

		ia = const_fold_val_i(exp_a);
		ib = const_fold_val_i(exp_b);

		return ia - ib;
	}
}

static int decl_cmp_ctor_pri(const void *a, const void *b)
{
	return decl_cmp_ctor_dtor_pri(a, b, attr_constructor);
}

static int decl_cmp_dtor_pri(const void *a, const void *b)
{
	return decl_cmp_ctor_dtor_pri(a, b, attr_destructor);
}

static void gen_inits_terms1(
		decl **ar,
		void (*declare)(decl *),
		int (*cmp)(const void *, const void *))
{
	size_t n = dynarray_count(ar);
	decl **i;

	if(!n)
		return;

	qsort(ar, n, sizeof *ar, cmp);

	for(i = ar; *i; i++)
		declare(*i);
}

void gen_inits_terms(decl **inits, decl **terms, out_ctx *octx)
{
	/*
	 * clang registers (via atexit or __cxa_atexit) destructors in a ctor now
	 * controllable by -fregister-global-dtors-with-atexit and -fuse-cxa-atexit
	 * (llvm commit 617e26152dea71efc44a45f5ae034f15c92767f0)
	 *
	 * but this doesn't work with newer libSystem:s, so we use cxa_atexit:
	 */
	if(cc1_target_details.dtor_via_ctor_atexit && terms){
		decl this_fn = { 0 };
		decl cxa_atexit = { 0 };
		struct funcargs cxa_atexit_args = { 0 };
		const out_val *cxa_atexit_fn;
		const struct section *sec = &section_text;
		type *t_uintptr = type_nav_btype(cc1_type_nav, type_uintptr_t);

		this_fn.ref = type_func_of(
			type_nav_btype(cc1_type_nav, type_void),
			&cxa_atexit_args, /* (...) */
			NULL
		);
		this_fn.spel = "__register_dtors";

		/* void __cxa_atexit(void (*fn)(), void *dso_handle); */
		cxa_atexit.spel = "__cxa_atexit";
		cxa_atexit.ref = type_func_of(
			type_nav_btype(cc1_type_nav, type_void),
			&cxa_atexit_args, /* (...) */
			NULL
		);

		cxa_atexit_fn = gen_decl_addr(octx, &cxa_atexit);

		asm_switch_section(sec);

		out_perfunc_init(octx, &this_fn, decl_asm_spel(&this_fn));
		out_func_prologue(octx, /*nargs*/0, /*variadic*/0, /*stackprot*/0, /*args*/NULL);

		for(decl **i = terms; i && *i; i++){
			decl *d = *i;
			const out_val *ident = gen_decl_addr(octx, d);
			const out_val *null = out_new_l(octx, t_uintptr, 0);
			const out_val *args[] = { ident, null, NULL };

			out_val_consume(octx, gen_call(NULL, &cxa_atexit, out_val_retain(octx, cxa_atexit_fn), args, octx, &d->where));
		}

		out_val_release(octx, cxa_atexit_fn);

		out_func_epilogue(
			octx,
			this_fn.ref,
			/*func_begin: where*/NULL,
			/*end_dbg_lbl*/NULL,
			sec
		);

		out_perfunc_teardown(octx);

		asm_declare_constructor(&this_fn);
	}else{
		gen_inits_terms1(terms, asm_declare_destructor,  decl_cmp_dtor_pri);
	}

	gen_inits_terms1(inits, asm_declare_constructor, decl_cmp_ctor_pri);
}
