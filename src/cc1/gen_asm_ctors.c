#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "../util/dynarray.h"

#include "decl.h"
#include "const.h"

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

void gen_inits_terms(decl **inits, decl **terms)
{
	gen_inits_terms1(inits, asm_declare_constructor, decl_cmp_ctor_pri);
	gen_inits_terms1(terms, asm_declare_destructor,  decl_cmp_dtor_pri);
}
