#include "ops.h"
#include "expr_assign_compound.h"

const char *str_expr_assign_compound()
{
	return "assign_compound";
}

void fold_expr_assign_compound(expr *e, symtable *stab)
{
#define lvalue e->lhs

	fold_inc_writes_if_sym(lvalue, stab);

	fold_expr_no_decay(e->lhs, stab);
	FOLD_EXPR(e->rhs, stab);

	fold_check_expr(e->lhs, FOLD_CHK_NO_ST_UN, "compound assignment");
	fold_check_expr(e->rhs, FOLD_CHK_NO_ST_UN, "compound assignment");

	/* skip the addr we inserted */
	if(!expr_must_lvalue(lvalue, "compound assignment")){
		/* prevent ICE from type_size(vla), etc */
		e->tree_type = lvalue->tree_type;
		return;
	}

	expr_assign_const_check(lvalue, &e->where);

	fold_check_restrict(lvalue, e->rhs, "compound assignment", &e->where);

	UCC_ASSERT(op_can_compound(e->op), "non-compound op in compound expr");

	/*expr_promote_int_if_smaller(&e->lhs, stab);
	 * lhs int promotion is handled in code-gen */
	expr_promote_int_if_smaller(&e->rhs, stab);

	{
		type *tlhs, *trhs;
		type *resolved = op_required_promotion(e->op, lvalue, e->rhs, &e->where, &tlhs, &trhs);

		if(tlhs){
			/* must cast the lvalue, then down cast once the operation is done
			 * special handling for expr_kind(e->lhs, cast) is done in the gen-code
			 */
			e->bits.compound_upcast_ty = tlhs;

		}else if(trhs){
			fold_insert_casts(trhs, &e->rhs, stab);
		}

		e->tree_type = lvalue->tree_type;

		(void)resolved;
		/*type_free_1(resolved); XXX: memleak */
	}

	/* type check is done in op_required_promotion() */
#undef lvalue
}

const out_val *gen_expr_assign_compound(const expr *e, out_ctx *octx)
{
	/* int += float
	 * lea int, cast up to float, add, cast down to int, store
	 */
	const out_val *saved_post = NULL, *addr_lhs, *rhs, *lhs, *result;

	addr_lhs = gen_expr(e->lhs, octx);

	out_val_retain(octx, addr_lhs); /* 2 */

	if(e->assign_is_post){
		out_val_retain(octx, addr_lhs); /* 3 */
		saved_post = out_deref(octx, addr_lhs); /* addr_lhs=2, saved_post=1 */
	}

	/* delay the dereference until after generating rhs.
	 * this is fine, += etc aren't sequence points
	 */

	rhs = gen_expr(e->rhs, octx);

	/* here's the delayed dereference */
	lhs = out_deref(octx, addr_lhs); /* addr_lhs=1 */
	if(e->bits.compound_upcast_ty)
		lhs = out_cast(octx, lhs, e->bits.compound_upcast_ty, /*normalise_bool:*/1);

	result = out_op(octx, e->op, lhs, rhs);
	gen_op_trapv(e->tree_type, &result, octx);

	if(e->bits.compound_upcast_ty) /* need to cast back down to store */
		result = out_cast(octx, result, e->tree_type, /*normalise_bool:*/1);

	if(!saved_post)
		out_val_retain(octx, result);
	out_store(octx, addr_lhs, result);

	if(!saved_post)
		return result;
	return saved_post;
}

const out_val *gen_expr_str_assign_compound(const expr *e, out_ctx *octx)
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

	UNUSED_OCTX();
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

const out_val *gen_expr_style_assign_compound(const expr *e, out_ctx *octx)
{
	IGNORE_PRINTGEN(gen_expr(e->lhs->lhs, octx));
	stylef(" %s= ", op_to_str(e->op));
	IGNORE_PRINTGEN(gen_expr(e->rhs, octx));
	return NULL;
}
