#include "ops.h"
#include "expr_assign_compound.h"

const char *str_expr_assign_compound()
{
	return "assign_compound";
}

void fold_expr_assign_compound(expr *e, symtable *stab)
{
	expr *const lvalue = e->lhs;

	fold_inc_writes_if_sym(lvalue, stab);

	fold_expr_no_decay(e->lhs, stab);
	FOLD_EXPR(e->rhs, stab);

	/* skip the addr we inserted */
	expr_must_lvalue(lvalue);

	if(type_ref_is_const(lvalue->tree_type))
		die_at(&e->where, "can't modify const expression %s", lvalue->f_str());

	fold_check_restrict(lvalue, e->rhs, "compound assignment", &e->where);

	UCC_ASSERT(op_can_compound(e->op), "non-compound op in compound expr");

	{
		type_ref *tlhs, *trhs;
		type_ref *resolved = op_required_promotion(e->op, lvalue, e->rhs, &e->where, &tlhs, &trhs);

		if(tlhs){
			/* can't cast the lvalue - we must cast the rhs to the correct size  */
			fold_insert_casts(lvalue->tree_type, &e->rhs, stab);

		}else if(trhs){
			fold_insert_casts(trhs, &e->rhs, stab);
		}

		e->tree_type = lvalue->tree_type;

		fold_check_expr(e, FOLD_CHK_NO_ST_UN, "compound assignment");

		(void)resolved;
		/*type_ref_free_1(resolved); XXX: memleak */
	}

	/* type check is done in op_required_promotion() */
}

basic_blk *gen_expr_assign_compound(expr *e, basic_blk *bb)
{
	bb = lea_expr(e->lhs, bb);

	if(e->assign_is_post){
		out_dup(bb);
		out_deref(bb);
		out_flush_volatile(bb);
		out_swap(bb);
		out_comment(bb, "saved for compound op");
	}

	out_dup(bb);
	/* delay the dereference until after generating rhs.
	 * this is fine, += etc aren't sequence points
	 */

	bb = gen_expr(e->rhs, bb);

	/* here's the delayed dereference */
	out_swap(bb), out_deref(bb), out_swap(bb);

	out_op(bb, e->op);

	out_store(bb);

	if(e->assign_is_post)
		out_pop(bb);

	return bb;
}

basic_blk *gen_expr_str_assign_compound(expr *e, basic_blk *bb)
{
	idt_printf("compound %s%s-assignment expr:\n",
			e->assign_is_post ? "post-" : "",
			op_to_str(e->op));

	idt_printf("assign to:\n");
	gen_str_indent++;
	print_expr(e->lhs);
	gen_str_indent--;
	idt_printf("assign from:\n");
	gen_str_indent++;
	print_expr(e->rhs);
	gen_str_indent--;

	return bb;
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

basic_blk *gen_expr_style_assign_compound(expr *e, basic_blk *bb)
{
	bb = gen_expr(e->lhs->lhs, bb);
	stylef(" %s= ", op_to_str(e->op));
	bb = gen_expr(e->rhs, bb);
	return bb;
}
