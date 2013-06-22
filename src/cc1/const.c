#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "data_structs.h"
#include "const.h"
#include "sym.h"
#include "../util/util.h"
#include "cc1.h"

void const_fold(expr *e, consty *k)
{
	/* always const_fold functions, i.e. builtins */
	const int should_fold = (fopt_mode & FOPT_CONST_FOLD) || expr_kind(e, funcall);

	k->type = CONST_NO;

	if(should_fold && e->f_const_fold){
		if(!e->const_eval.const_folded){
			e->const_eval.const_folded = 1;
			e->f_const_fold(e, &e->const_eval.k);
		}

		memcpy(k, &e->const_eval.k, sizeof *k);
	}
}

#if 0
int const_expr_is_const(expr *e)
{
	/* TODO: move this to the expr ops */
	if(expr_kind(e, cast))
		return const_expr_is_const(e->expr);

	if(expr_kind(e, val) || expr_kind(e, sizeof) || expr_kind(e, addr))
		return 1;

	/* function names are */
	if(expr_kind(e, identifier) && decl_is_func(e->tree_type))
		return 1;

	/*
	 * const int i = 5; is const
	 * but
	 * const int i = *p; is not const
	 */
	return 0;
	if(e->sym)
		return decl_is_const(e->sym->decl);

	return decl_is_const(e->tree_type);
}
#endif

static int const_expr_zero(expr *e, int zero)
{
	consty k;

	const_fold(e, &k);

	if(k.type != CONST_NUM)
		return 0;

	if(K_FLOATING(k.bits.num))
		return !k.bits.num.val.f == zero;

	return !k.bits.num.val.i == zero;
}

void const_fold_integral(expr *e, numeric *piv)
{
	consty k;
	const_fold(e, &k);

	UCC_ASSERT(k.type == CONST_NUM, "not const");
	UCC_ASSERT(k.offset == 0, "got offset for val?");
	UCC_ASSERT(K_INTEGRAL(k.bits.num), "fp?");

	memcpy_safe(piv, &k.bits.num);
}

integral_t const_fold_val_i(expr *e)
{
	numeric num;
	const_fold_integral(e, &num);
	return num.val.i;
}

int const_expr_and_non_zero(expr *e)
{
	return const_expr_zero(e, 0);
}

int const_expr_and_zero(expr *e)
{
	return const_expr_zero(e, 1);
}

/*
long const_expr_value(expr *e)
{
	enum constyness k;
	numeric val;

	const_fold(e, &val, &k);
	UCC_ASSERT(k == CONST_WITH_VAL, "not a constant");

	return val.val;
}
*/
