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

	fold_check_expr(e->lhs, FOLD_CHK_NO_ST_UN, "compound assignment");
	fold_check_expr(e->rhs, FOLD_CHK_NO_ST_UN, "compound assignment");

	/* skip the addr we inserted */
	expr_must_lvalue(lvalue);

	expr_assign_const_check(lvalue, &e->where);

	fold_check_restrict(lvalue, e->rhs, "compound assignment", &e->where);

	UCC_ASSERT(op_can_compound(e->op), "non-compound op in compound expr");

	{
		type *tlhs, *trhs;
		type *resolved = op_required_promotion(e->op, lvalue, e->rhs, &e->where, &tlhs, &trhs);

		if(tlhs){
			/* must cast the lvalue, then down cast once the operation is done
			 * special handling for expr_kind(e->lhs, cast) is done in the gen-code
			 */
			fold_insert_casts(tlhs, &e->lhs, stab);

			/* casts may be inserted anyway, and don't want to rely on
			 * .implicit_cast stuff */
			e->bits.compound_upcast = 1;

		}else if(trhs){
			fold_insert_casts(trhs, &e->rhs, stab);
		}

		e->tree_type = lvalue->tree_type;

		(void)resolved;
		/*type_free_1(resolved); XXX: memleak */
	}

	/* type check is done in op_required_promotion() */
}

void gen_expr_assign_compound(expr *e)
{
	/* int += float
	 * lea int, cast up to float, add, cast down to int, store
	 */
	lea_expr(e->bits.compound_upcast ? expr_cast_child(e->lhs) : e->lhs);

	if(e->assign_is_post){
		out_dup();
		out_deref();
		out_flush_volatile();
		out_swap();
		out_comment("saved for compound op");
	}

	out_dup();
	/* delay the dereference until after generating rhs.
	 * this is fine, += etc aren't sequence points
	 */

	gen_expr(e->rhs);

	/* here's the delayed dereference */
	out_swap();
	out_deref();
	if(e->bits.compound_upcast)
		out_cast(e->lhs->tree_type, /*normalise_bool:*/1);
	out_swap();

	out_op(e->op);

	if(e->bits.compound_upcast) /* need to cast back down to store */
		out_cast(e->tree_type, /*normalise_bool:*/1);

	out_store();

	if(e->assign_is_post)
		out_pop();
}

void gen_expr_str_assign_compound(expr *e)
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

void gen_expr_style_assign_compound(expr *e)
{
	gen_expr(e->lhs->lhs);
	stylef(" %s= ", op_to_str(e->op));
	gen_expr(e->rhs);
}
