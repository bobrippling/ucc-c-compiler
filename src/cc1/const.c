#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "const.h"
#include "sym.h"
#include "../util/util.h"
#include "cc1.h"

/* returns 0 if successfully folded */
int const_fold(expr *e)
{
	if(fopt_mode & FOPT_CONST_FOLD && e->f_const_fold)
		return e->f_const_fold(e);

	return 1;
}

int const_expr_is_const(expr *e)
{
	/* TODO: move this to the expr ops */
	if(expr_kind(e, cast))
		return const_expr_is_const(e->expr);

	if(expr_kind(e, val) || expr_kind(e, sizeof) || expr_kind(e, addr))
		return 1;

	/*
	 * const int i = 5; is const
	 * but
	 * const int i = *p; is not const
	 */
	return 0;
#if 0
	if(e->sym)
		return decl_is_const(e->sym->decl);

	return decl_is_const(e->tree_type);
#endif
}

int const_expr_is_zero(expr *e)
{
	if(expr_kind(e, cast))
		return const_expr_is_zero(e->expr);

	return const_expr_is_const(e) && (expr_kind(e, val) ? e->val.iv.val == 0 : 0);
}
