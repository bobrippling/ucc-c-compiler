#include "ops.h"

const char *str_expr_assign_compound()
{
	return "assign_compound";
}

void fold_expr_assign_compound(expr *e, symtable *stab)
{
	int type_ok;

	fold_inc_writes_if_sym(e->lhs, stab);

	{
		expr *addr = expr_new_addr();

		addr->lhs = e->lhs;

		e->lhs = addr;
		/* take the address of where we're assigning to - only eval once */
	}

	fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);

	fold_coerce_assign(e->lhs->tree_type, e->rhs, &type_ok);

	/* skip the addr we inserted */
	if(!expr_is_lvalue(e->lhs->lhs, 0)){
		DIE_AT(&e->lhs->where, "compound target not an lvalue (%s)",
				e->lhs->f_str());
	}

	if(decl_is_const(e->lhs->tree_type))
		DIE_AT(&e->where, "can't modify const expression %s", e->lhs->f_str());

	UCC_ASSERT(op_can_compound(e->op), "non-compound op in compound expr");

	e->tree_type = decl_copy(e->lhs->lhs->tree_type);
}

void gen_expr_assign_compound(expr *e, symtable *stab)
{
	fold_disallow_st_un(e, "copy (TODO)"); /* yes this is meant to be in gen */

	gen_expr(e->lhs, stab);

	if(e->assign_is_post){
		out_dup();
		out_op_unary(op_deref);
		out_flush_volatile();
		out_swap();
		out_comment("saved for compound op");
	}

	out_dup();
	out_op_unary(op_deref);

	gen_expr(e->rhs, stab);

	out_op(e->op);

	out_store();

	if(e->assign_is_post)
		out_pop();
}

void gen_expr_str_assign_compound(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("compound assignment, expr:\n", e->assign_is_post ? "post-inc/dec " : "");
	idt_printf("assign to:\n");
	gen_str_indent++;
	print_expr(e->lhs->lhs); /* skip our addr */
	gen_str_indent--;
	idt_printf("assign from:\n");
	gen_str_indent++;
	print_expr(e->rhs);
	gen_str_indent--;
}

void mutate_expr_assign_compound(expr *e)
{
	e->freestanding = 1;
}

expr *expr_new_assign_compound(expr *to, expr *from, enum op_type op)
{
	expr *e = expr_new_wrapper(assign_compound);

	e->lhs = to;
	e->rhs = from;
	e->op = op;

	return e;
}

void gen_expr_style_assign_compound(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
