#include "../tokconv.h"
#include "../parse_type.h"
#include "../../util/platform.h"

/* va_start */
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
	if(!expr_is_lvalue(va_l))
		DIE_AT(&e->where, "%s argument 1 must be an lvalue (%s)",
				BUILTIN_SPEL(e->expr), va_l->f_str());

	if(!type_ref_is_type(e->funcargs[0]->tree_type, type_va_list)){
		DIE_AT(&e->funcargs[0]->where,
				"first argument to %s should be a va_list",
				BUILTIN_SPEL(e->expr));
	}

	/* TODO: check it's the last argument to the function */
	/* TODO: check we're in a variadic function */

	e->tree_type = type_ref_new_VOID();
}

static void gen_va_start(expr *e, symtable *stab)
{
	/* assign to
	 *   e->funcargs[0]
	 * from
	 *   __builtin_frame_address(0) - pws() * 2
	 */
	out_push_frame_ptr(0);
	out_push_i(type_ref_new_INTPTR_T(), platform_word_size() * 2);
	/* stack grows down,
	 * we want the args which are above the saved return addr + FP
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
	ICE("TODO");
}

static void fold_va_arg(expr *e, symtable *stab)
{
	ICE("TODO");
}

expr *parse_va_arg(void)
{
	/* va_arg(list, type) */
	expr *fcall = expr_new_funcall();
	expr *list = parse_expr_no_comma();
	type_ref *ty;

	EAT(token_comma);
	ty = parse_type();

	fcall->expr = list;
	fcall->bits.tref = ty;

	expr_mutate_builtin_gen(fcall, va_arg);

	return fcall;
}

/* va_end */

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
