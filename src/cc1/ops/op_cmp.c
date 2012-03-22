
/* called by gt, lt, eq, ... */
fold_op_cmp(op *o)
{
	lhs_su = e->op->lhs->tree_type->type->spec & spec_unsigned ? UNSIGNED : SIGNED;
	rhs_su = e->op->rhs->tree_type->type->spec & spec_unsigned ? UNSIGNED : SIGNED;

	if(op_is_cmp(e->op) && rhs_su != lhs_su){
		/*
			* assert(lhs == UNSIGNED);
			* vals default to signed, change to unsigned
			*/

		if(expr_kind(RHS, val) && RHS->val.iv.val >= 0){
			UCC_ASSERT(lhs_su == UNSIGNED, "signed-unsigned assumption failure");
			RHS->tree_type->type->spec |= spec_unsigned;
		}else if(expr_kind(LHS, val) && LHS->val.iv.val >= 0){
			UCC_ASSERT(rhs_su == UNSIGNED, "signed-unsigned assumption failure");
			LHS->tree_type->type->spec |= spec_unsigned;
		}else{
				cc1_warn_at(&e->where, 0, WARN_SIGN_COMPARE, "comparison between signed and unsigned%s%s%s%s%s%s",
						SPEL_IF_IDENT(LHS), SPEL_IF_IDENT(RHS));
		}
	}
}

static void asm_compare(expr *e, symtable *tab)
{
	const char *cmp = NULL;

	gen_expr(e->lhs, tab);
	gen_expr(e->rhs, tab);
	asm_temp(1, "pop rbx");
	asm_temp(1, "pop rax");
	asm_temp(1, "xor rcx,rcx"); /* must be before cmp */
	asm_temp(1, "cmp rax,rbx");

	/* check for unsigned, since signed isn't explicitly set */
#define SIGNED(s, u) e->tree_type->type->spec & spec_unsigned ? u : s

	switch(e->op){
		case op_eq: cmp = "e";  break;
		case op_ne: cmp = "ne"; break;

		case op_le: cmp = SIGNED("le", "be"); break;
		case op_lt: cmp = SIGNED("l",  "b");  break;
		case op_ge: cmp = SIGNED("ge", "ae"); break;
		case op_gt: cmp = SIGNED("g",  "a");  break;

		default:
			ICE("asm_compare: unhandled comparison");
	}

	asm_temp(1, "set%s cl", cmp);
	asm_temp(1, "push rcx");
}

