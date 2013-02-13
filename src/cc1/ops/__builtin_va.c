#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../data_structs.h"
#include "../expr.h"
#include "__builtin.h"
#include "__builtin_va.h"

#include "../cc1.h"
#include "../fold.h"
#include "../gen_asm.h"
#include "../out/out.h"

#include "__builtin_va.h"

#include "../tokenise.h"
#include "../tokconv.h"
#include "../parse.h"
#include "../parse_type.h"

/*#include "../parse_type.h"
#include "../../util/platform.h"*/

/* va_start */
static void LVALUE_VA_CHECK(expr *va_l, expr *in)
{
	if(!expr_is_lvalue(va_l))
		DIE_AT(&in->where, "%s argument 1 must be an lvalue (%s)",
				BUILTIN_SPEL(in), va_l->f_str());
}

static void fold_va_start(expr *e, symtable *stab)
{
	expr *va_l;

	if(dynarray_count((void **)e->funcargs) != 2)
		DIE_AT(&e->where, "%s requires two arguments", BUILTIN_SPEL(e->expr));

	va_l = e->funcargs[0];
	fold_inc_writes_if_sym(va_l, stab);

	FOLD_EXPR_NO_DECAY(e->funcargs[0], stab);
	FOLD_EXPR(         e->funcargs[1], stab);

	va_l = e->funcargs[0];
	LVALUE_VA_CHECK(va_l, e->expr);

	if(!type_ref_is_type(e->funcargs[0]->tree_type, type_va_list)){
		DIE_AT(&e->funcargs[0]->where,
				"first argument to %s should be a va_list",
				BUILTIN_SPEL(e->expr));
	}

	/* TODO: check it's the last argument to the function */
	/* TODO: check we're in a variadic function */

	e->tree_type = type_ref_new_VOID();

	/* finally store the number of arguments to this function */
	{
		funcargs *args = type_ref_funcargs(curdecl_func->ref);
		int nargs = dynarray_count((void **)args->arglist);

		if(!args->variadic)
			DIE_AT(&e->where, "va_start in non-variadic function");

		e->bits.n = nargs;
	}
}

static void gen_va_start(expr *e, symtable *stab)
{
	/* assign to
	 *   e->funcargs[0]
	 * from
	 *   __builtin_frame_address(0) + pws() * (nargs + 1)
	 *
	 * +1 to step over the saved bp
	 */
	out_push_frame_ptr(0);
	out_push_i(type_ref_new_INTPTR_T(),
			(e->bits.n + 1) * platform_word_size());
	/* stack grows down,
	 * we want the args which are belog the saved return addr + FP
	 */
	out_op(op_plus);

	lea_expr(e->funcargs[0], stab);
	out_swap();
	out_store();
}

expr *parse_va_start(void)
{
	/* va_start(__builtin_va_list &, identifier)
	 * second argument may be any expression - we don't use it
	 */
	expr *fcall = parse_any_args();
	expr_mutate_builtin_gen(fcall, va_start);
	return fcall;
}

/* va_arg */

static void gen_va_arg(expr *e, symtable *stab)
{
	/* read from the va_list / char * */
	gen_expr(e->lhs, stab);
	out_dup();

	/* increment the va_list up the stack */
	out_push_i(type_ref_new_INTPTR_T(), platform_word_size());
	out_op(op_plus);

	/* store the new pointer value */
	lea_expr(e->lhs, stab);
	out_swap();
	out_store();
	out_pop(); /* pop the pointer value */

	/* still have the pointer on stack,
	 * deref it, size of type */
	out_change_type(
			type_ref_ptr_depth_inc(
				e->bits.tref));
	out_deref(); /* leave on the stack */
}

static void fold_va_arg(expr *e, symtable *stab)
{
	type_ref *const ty = e->bits.tref;

	FOLD_EXPR_NO_DECAY(e->lhs, stab);
	fold_type_ref(ty, NULL, stab);

	LVALUE_VA_CHECK(e->lhs, e->expr);

	if(!type_ref_is_type(e->lhs->tree_type, type_va_list))
		DIE_AT(&e->expr->where, "va_list expected for va_arg");

	e->tree_type = ty;
}

expr *parse_va_arg(void)
{
	/* va_arg(list, type) */
	expr *fcall = expr_new_funcall();
	expr *list = parse_expr_no_comma();
	type_ref *ty;

	EAT(token_comma);
	ty = parse_type();

	fcall->lhs = list;
	fcall->bits.tref = ty;

	expr_mutate_builtin_gen(fcall, va_arg);

	return fcall;
}

/* va_end */

#pragma GCC diagnostic ignored "-Wunused-parameter"

static void gen_va_end(expr *e, symtable *stab)
{
	ICE("TODO");
}

static void fold_va_end(expr *e, symtable *stab)
{
	ICE("TODO");
}

expr *parse_va_end(void)
{
	expr *fcall = parse_any_args();
	expr_mutate_builtin_gen(fcall, va_end);
	return fcall;
}
