#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../../util/alloc.h"
#include "ops.h"
#include "../out/asm.h"

const char *str_expr_cast()
{
	return "cast";
}

void fold_const_expr_cast(expr *e, intval *piv, enum constyness *type)
{
	const_fold(e->expr, piv, type);
}

void fold_expr_cast(expr *e, symtable *stab)
{
	int size_lhs, size_rhs;
	decl *dlhs, *drhs;

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

	dlhs = e->tree_type;
	drhs = e->expr->tree_type;

	if(!decl_is_void(dlhs) && (size_lhs = asm_type_size(dlhs)) < (size_rhs = asm_type_size(drhs))){
		char buf[DECL_STATIC_BUFSIZ];

		strcpy(buf, decl_to_str(drhs));

		cc1_warn_at(&e->where, 0, 1, WARN_LOSS_PRECISION,
				"possible loss of precision %s, size %d <-- %s, size %d",
				decl_to_str(dlhs), size_lhs,
				buf, size_rhs);
	}
}

void gen_expr_cast_1(expr *e, FILE *f)
{
	enum constyness type;
	intval iv;

	const_fold(e, &iv, &type);

	switch(type){
		case CONST_NO:
			ICE("bad cast static init");

		case CONST_WITH_VAL:
			/* output with possible truncation (truncate?) */
			asm_declare_out(f, e->tree_type, "%ld", iv.val);
			break;

		case CONST_WITHOUT_VAL:
			/* only possible if the cast-to and cast-from are the same size */

			if(decl_size(e->tree_type) != decl_size(e->expr->tree_type)){
				WARN_AT(&e->where,
						"%scast changes type size (not a load-time constant)",
						e->expr_cast_implicit ? "implicit " : ""
						);
			}

			e->expr->f_gen_1(e->expr, f);
			break;
	}
}

void gen_expr_cast(expr *e, symtable *stab)
{
	decl *dto, *dfrom;

	gen_expr(e->expr, stab);

	dto = e->tree_type;
	dfrom = e->expr->tree_type;

	/* return if cast-to-void */
	if(decl_is_void(dto)){
		out_change_decl(dto);
		out_comment("cast to void");
		return;
	}

	/* check float <--> int conversion */
	if(decl_is_float(dto) != decl_is_float(dfrom))
		ICE("TODO: float <-> int casting");

	out_cast(dfrom, dto);
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
