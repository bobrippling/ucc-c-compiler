#include <stdlib.h>
#include "ops.h"

const char *str_expr_cast()
{
	return "cast";
}

int fold_const_expr_cast(expr *e)
{
	return const_fold(e->expr);
}

void fold_expr_cast(expr *e, symtable *stab)
{
	fold_expr(e->expr, stab);

	fold_disallow_st_un(e->expr, "cast-expr");

	/*
	 * if we don't have a valid tree_type, get one
	 * this is only the case where we're involving a tdef or typeof
	 */

	if(e->tree_type->type->primitive == type_unknown){
		decl_free(e->tree_type);
		e->tree_type = decl_copy(e->expr->tree_type);
	}

	fold_decl(e->tree_type, stab); /* struct lookup, etc */

	fold_disallow_st_un(e, "cast-target");

#ifdef CAST_COLLAPSE
	if(expr_kind(e->expr, cast)){
		/* get rid of e->expr, replace with e->expr->rhs */
		expr *del = e->expr;

		e->expr = e->expr->expr;

		/*decl_free(del->tree_type); XXX: memleak */
		expr_free(del);

		fold_expr_cast(e, stab);
	}
#endif
}

void gen_expr_cast_1(expr *e, FILE *f)
{
	asm_declare_single_part(f, e->expr);
}

void gen_expr_cast(expr *e, symtable *stab)
{
	/* ignore the lhs, it's just a type spec */
	/* FIXME: size changing? */
	gen_expr(e->expr, stab);
}

void gen_expr_str_cast(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("cast expr:\n");
	gen_str_indent++;
	print_expr(e->expr);
	gen_str_indent--;
}

void mutate_expr_cast(expr *e)
{
	e->f_const_fold = fold_const_expr_cast;
	e->f_gen_1      = gen_expr_cast_1;
}

expr *expr_new_cast(decl *to)
{
	expr *e = expr_new_wrapper(cast);
	e->tree_type = to;
	return e;
}

void gen_expr_style_cast(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
