#include "ops.h"

const char *str_expr_assign_compound()
{
	return "assign_compound";
}

void fold_expr_assign_compound(expr *e, symtable *stab)
{
	expr *const lvalue = e->lhs;

	{
		expr *addr = expr_new_addr();

		addr->lhs = e->lhs;

		e->lhs = addr;
		/* take the address of where we're assigning to - only eval once */
	}

	fold_inc_writes_if_sym(lvalue, stab);

	fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);

	/* skip the addr we inserted */
	if(!expr_is_lvalue(lvalue, 0)){
		DIE_AT(&lvalue->where, "compound target not an lvalue (%s)",
				lvalue->f_str());
	}

	if(decl_is_const(lvalue->tree_type))
		DIE_AT(&e->where, "can't modify const expression %s", lvalue->f_str());

	UCC_ASSERT(op_can_compound(e->op), "non-compound op in compound expr");

	/* pass the addr's target to promote_types */
	e->tree_type = op_promote_types(e->op, &e->lhs->lhs, &e->rhs, &e->where, stab);

	/* type check */
	fold_decl_equal(e->lhs->lhs->tree_type, e->rhs->tree_type,
			&e->where, WARN_ASSIGN_MISMATCH,
			"compound-assignment type mismatch");

	/* FIXME: insert cast to lhs? */
}

void gen_expr_assign_compound(expr *e, symtable *stab)
{
	fold_disallow_st_un(e, "copy (TODO)"); /* yes this is meant to be in gen */

	gen_expr(e->lhs, stab);

	if(e->assign_is_post){
		out_dup();
		out_deref();
		out_flush_volatile();
		out_swap();
		out_comment("saved for compound op");
	}

	out_dup();
	out_deref();

	gen_expr(e->rhs, stab);

	out_op(e->op);

	out_store();

	if(e->assign_is_post)
		out_pop();
}

void gen_expr_str_assign_compound(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("compound %s%s-assignment expr:\n",
			e->assign_is_post ? "post-" : "",
			op_to_str(e->op));

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
