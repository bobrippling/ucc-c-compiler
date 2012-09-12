#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "const.h"
#include "sym.h"
#include "../util/util.h"
#include "cc1.h"

void const_fold(expr *e, intval *iv, enum constyness *success)
{
	/* always const_fold functions, i.e. builtins */
	const int should_fold = (fopt_mode & FOPT_CONST_FOLD) || expr_kind(e, funcall);

	*success = CONST_NO;

	if(should_fold && e->f_const_fold)
		e->f_const_fold(e, iv, success);
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

int const_expr_and_zero(expr *e)
{
	enum constyness k;
	intval val;

	const_fold(e, &val, &k);

	return k == CONST_WITH_VAL && val.val == 0;
}

/*
long const_expr_value(expr *e)
{
	enum constyness k;
	intval val;

	const_fold(e, &val, &k);
	UCC_ASSERT(k == CONST_WITH_VAL, "not a constant");

	return val.val;
}
*/
