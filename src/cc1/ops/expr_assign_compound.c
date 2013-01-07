#include "ops.h"
#include "expr_assign_compound.h"

const char *str_expr_assign_compound()
{
	return "assign_compound";
}

void fold_expr_assign_compound(expr *e, symtable *stab)
{
	expr *const lvalue = e->lhs;

	{
		expr *addr = expr_new_addr(e->lhs);

		e->lhs = addr;
		/* take the address of where we're assigning to - only eval once */
	}

	fold_inc_writes_if_sym(lvalue, stab);

	FOLD_EXPR(e->lhs, stab);
	FOLD_EXPR(e->rhs, stab);

	/* skip the addr we inserted */
	if(!expr_is_lvalue(lvalue)){
		DIE_AT(&lvalue->where, "compound target not an lvalue (%s)",
				lvalue->f_str());
	}

	if(type_ref_is_const(lvalue->tree_type))
		DIE_AT(&e->where, "can't modify const expression %s", lvalue->f_str());

	fold_check_restrict(lvalue, e->rhs, "compound assignment", &e->where);

	UCC_ASSERT(op_can_compound(e->op), "non-compound op in compound expr");

	{
		type_ref *tlhs, *trhs;
		type_ref *resolved = op_required_promotion(e->op, lvalue, e->rhs, &e->where, &tlhs, &trhs);

		if(tlhs){
			/* can't cast the lvalue - we must cast the rhs to the correct size  */

			if(tlhs != lvalue->tree_type)
				type_ref_free_1(tlhs);

			fold_insert_casts(lvalue->tree_type, &e->rhs, stab, &e->where, op_to_str(e->op));

		}else if(trhs){
			fold_insert_casts(trhs, &e->rhs, stab, &e->where, op_to_str(e->op));
		}

		e->tree_type = lvalue->tree_type;

		(void)resolved;
		/*type_ref_free_1(resolved); XXX: memleak */
	}

	/* type check is done in op_required_promotion() */
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
