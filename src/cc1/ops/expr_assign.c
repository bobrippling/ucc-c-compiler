#include "ops.h"

const char *str_expr_assign()
{
	return "assign";
}

int fold_const_expr_assign(expr *e)
{
	(void)e;
	return 1; /* could check if the assignment subtree is const */
}

static int is_lvalue(expr *e)
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

	if(expr_kind(e, identifier))
		return decl_is_callable(e->tree_type) ? decl_is_func_ptr(e->tree_type) : 1;

	if(expr_kind(e, op) && e->op->f_store)
		return 1;
		/*switch(e->op){
			case op_deref:
			case op_struct_ptr:
			case op_struct_dot:
				return 1;
			default:
				break;
		}*/

	return 0;
}

void fold_expr_assign(expr *e, symtable *stab)
{
	fold_inc_writes_if_sym(e->assign_to, stab);

	fold_expr(e->assign_to, stab);
	fold_expr(e->expr, stab);

	/* wait until we get the tree types, etc */
	if(!is_lvalue(e->assign_to))
		die_at(&e->assign_to->where, "not an lvalue (%s%s%s)",
				e->assign_to->f_str(),
				expr_kind(e->assign_to, op) ? " - " : "",
				expr_kind(e->assign_to, op) ? e->assign_to->op->f_str() : ""
			);

	if(decl_is_const(e->assign_to->tree_type)){
		/* allow const init: */
		if(e->assign_to->sym->decl->init != e->expr)
			die_at(&e->where, "can't modify const expression");
	}


	if(e->assign_to->sym)
		/* read the tree_type from what we're assigning to, not the expr */
		e->tree_type = decl_copy(e->assign_to->sym->decl);
	else
		e->tree_type = decl_copy(e->assign_to->tree_type);

	fold_typecheck(e->assign_to, e->expr, stab, &e->where);

	/* type check */
	fold_decl_equal(e->assign_to->tree_type, e->expr->tree_type,
		&e->where, WARN_ASSIGN_MISMATCH,
				"assignment type mismatch%s%s%s",
				e->assign_to->spel ? " (" : "",
				e->assign_to->spel ? e->assign_to->spel : "",
				e->assign_to->spel ? ")" : "");
}

void gen_expr_assign(expr *e, symtable *stab)
{
	if(e->assign_is_post){
		/* if this is the case, ->expr->expr is ->assign_to, and ->expr->expr2 is an addition/subtraction of 1 * something */
		gen_expr(e->assign_to, stab);
		asm_temp(1, "; save previous for post assignment");
	}

	gen_expr(e->expr, stab);
#ifdef USE_MOVE_RAX_RSP
	asm_temp(1, "mov rax, [rsp]");
#endif

	UCC_ASSERT(e->assign_to->f_store, "invalid store expression %s (no f_store())", e->assign_to->f_str());

	/* store back to the sym's home */
	e->assign_to->f_store(e->assign_to, stab);

	if(e->assign_is_post){
		asm_temp(1, "pop rax ; the value from ++/--");
		asm_temp(1, "mov rax, [rsp] ; the value we saved");
	}
}

void gen_expr_str_assign(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("%sassignment, expr:\n", e->assign_is_post ? "post-inc/dec " : "");
	idt_printf("assign to:\n");
	gen_str_indent++;
	print_expr(e->assign_to);
	gen_str_indent--;
	idt_printf("assign from:\n");
	gen_str_indent++;
	print_expr(e->expr);
	gen_str_indent--;
}

expr *expr_new_assign(expr *to, expr *from)
{
	expr *e = expr_new_wrapper(assign);

	e->assign_to = to;
	e->expr = from;

	e->f_const_fold = fold_const_expr_assign;
	e->freestanding = 1;

	return e;
}
