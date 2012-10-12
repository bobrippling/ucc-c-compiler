#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../../util/alloc.h"
#include "ops.h"
#include "expr_cast.h"
#include "../out/asm.h"

const char *str_expr_cast()
{
	return "cast";
}

void fold_const_expr_cast(expr *e, intval *piv, enum constyness *type)
{
	const_fold(e->expr, piv, type);
}

void fold_expr_cast_descend(expr *e, symtable *stab, int descend)
{
	int size_lhs, size_rhs;
	decl *dlhs, *drhs;

	if(descend)
		fold_expr(e->expr, stab);

	e->tree_type = e->decl;
	fold_decl(e->tree_type, stab); /* struct lookup, etc */

	fold_disallow_st_un(e->expr, "cast-expr");
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

#ifdef W_QUAL
	if(decl_is_ptr(dlhs) && decl_is_ptr(drhs) && (dlhs->type->qual | drhs->type->qual) != dlhs->type->qual){
		const enum type_qualifier away = drhs->type->qual & ~dlhs->type->qual;
		char *buf = type_qual_to_str(away);
		char *p;

		p = &buf[strlen(buf)-1];
		if(p >= buf && *p == ' ')
			*p = '\0';

		WARN_AT(&e->where, "casting away qualifiers (%s)", buf);
	}
#endif
}

void fold_expr_cast(expr *e, symtable *stab)
{
	fold_expr_cast_descend(e, stab, 1);
}

void static_expr_cast_store(expr *e)
{
	enum constyness type;
	intval iv;

	const_fold(e, &iv, &type);

	switch(type){
		case CONST_NO:
			ICE("bad cast static init");

		case CONST_WITH_VAL:
			/* output with possible truncation (truncate?) */
			asm_declare_partial("%ld", iv.val);
			break;

		case CONST_WITHOUT_VAL:
		{
			int from_sz, to_sz;
			/* only possible if the cast-to and cast-from are the same size */

			from_sz = decl_size(e->expr->tree_type);
			to_sz = decl_size(e->tree_type);

			if(to_sz != from_sz){
				WARN_AT(&e->where,
						"%scast changes type size %d -> %d (not a load-time constant)",
						e->expr_cast_implicit ? "implicit " : "",
						from_sz, to_sz);
			}

			static_store(e->expr);
			break;
		}
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
	if(decl_is_floating(dto) != decl_is_floating(dfrom))
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
	e->f_static_addr = static_expr_cast_store;
}

expr *expr_new_cast(decl *to, int implicit)
{
	expr *e = expr_new_wrapper(cast);
	e->decl = to;
	e->expr_cast_implicit = implicit;
	return e;
}

void gen_expr_style_cast(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
