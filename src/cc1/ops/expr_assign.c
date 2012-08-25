#include "ops.h"

const char *str_expr_assign()
{
	return "assign";
}

int expr_is_lvalue(expr *e, enum lvalue_opts opts)
{
	/*
	 * valid lvaluess:
	 *
	 *   x              = 5;
	 *   *(expr)        = 5;
	 *   struct.member  = 5;
	 *   struct->member = 5;
	 * and so on
	 *
	 * also can't be const, checked in fold_assign (since we allow const inits)
	 */

	if(decl_is_array(e->tree_type))
		return opts & LVAL_ALLOW_ARRAY;

	if(expr_kind(e, identifier))
		return (opts & LVAL_ALLOW_FUNC) || !e->tree_type->func_code;

	if(expr_kind(e, deref))
		return 1;

	if(expr_kind(e, struct))
		return 1;

	return 0;
}

void fold_expr_assign(expr *e, symtable *stab)
{
	int type_ok;

	fold_inc_writes_if_sym(e->lhs, stab);

	fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);

	if(expr_kind(e->lhs, identifier))
		e->lhs->sym->nreads--; /* cancel the read that fold_ident thinks it got */

	if(decl_is_void(e->rhs->tree_type))
		DIE_AT(&e->where, "assignment from void expression");

	fold_coerce_assign(e->lhs->tree_type, e->rhs, &type_ok);

	if(!expr_is_lvalue(e->lhs, LVAL_ALLOW_ARRAY)){
		/* only allow assignments to type[] if it's an init */
		DIE_AT(&e->lhs->where, "not an lvalue (%s%s%s)",
				e->lhs->f_str(),
				expr_kind(e->lhs, op) ? " - " : "",
				expr_kind(e->lhs, op) ? op_to_str(e->lhs->op) : ""
			);
	}

	if(decl_is_const(e->lhs->tree_type)){
		/* allow const init: */
		sym *const sym = e->lhs->sym;
		if(!sym || sym->decl->init != e->rhs)
			DIE_AT(&e->where, "can't modify const expression %s", e->lhs->f_str());
	}


	e->tree_type = decl_copy(e->lhs->tree_type);

	/* type check */
	if(!type_ok){
		fold_decl_equal(e->lhs->tree_type, e->rhs->tree_type,
				&e->where, WARN_ASSIGN_MISMATCH,
				"assignment type mismatch%s%s%s",
				e->lhs->spel ? " (" : "",
				e->lhs->spel ? e->lhs->spel : "",
				e->lhs->spel ? ")" : "");
	}

	/* do the typecheck after the equal-check, since the typecheck inserts casts */
	fold_insert_casts(e->lhs->tree_type, &e->rhs, stab, &e->where, "assignment");
}

void gen_expr_assign(expr *e, symtable *stab)
{
	/*if(decl_is_struct_or_union(e->tree_type))*/
	fold_disallow_st_un(e, "copy (TODO)"); /* yes this is meant to be in gen */

	UCC_ASSERT(!e->assign_is_post, "assign_is_post set for non-compound assign");

	lea_expr(e->lhs, stab);
	gen_expr(e->rhs, stab);
	out_store();
}

void gen_expr_str_assign(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("assignment, expr:\n");
	idt_printf("assign to:\n");
	gen_str_indent++;
	print_expr(e->lhs);
	gen_str_indent--;
	idt_printf("assign from:\n");
	gen_str_indent++;
	print_expr(e->rhs);
	gen_str_indent--;
}

void mutate_expr_assign(expr *e)
{
	e->freestanding = 1;
}

expr *expr_new_assign(expr *to, expr *from)
{
	expr *ass = expr_new_wrapper(assign);

	ass->lhs = to;
	ass->rhs = from;

	return ass;
}

void gen_expr_style_assign(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
