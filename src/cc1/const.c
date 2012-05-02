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
	if((fopt_mode & FOPT_CONST_FOLD) == 0)
		return 1;

	if(e->f_const_fold)
		return e->f_const_fold(e);

	return 0;
}

int const_expr_is_const(expr *e)
{
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

int const_expr_val(expr *e)
{
	if(expr_kind(e, val))
		return e->val.iv.val; /* FIXME: doesn't account for longs */

	if(expr_kind(e, sizeof))
		ICE("TODO: const_expr_val with sizeof");

	if(expr_kind(e, addr))
		die_at(&e->where, "address of expression can't be resolved at compile-time");

	ICE("const_expr_val on non-const expr");
	return 0;
}

int const_expr_is_zero(expr *e)
{
	if(expr_kind(e, cast))
		return const_expr_is_zero(e->expr);

	return const_expr_is_const(e) && (expr_kind(e, val) ? e->val.iv.val == 0 : 0);
}
