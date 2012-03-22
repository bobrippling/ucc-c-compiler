int exec_op_plus(op)
{
	if(rhs)
		return lhs->val.iv.val + rhs->val.iv.val;
	return lhs->val.iv.val;
}

void optimise_op_plus(op)
{
#define VAL(e, x) (expr_kind(e, val) && e->val.iv.val == x)
	if(VAL(e->lhs, 0) || (e->rhs ? VAL(e->rhs, 0) : 0))
		POSSIBLE_OPT(e, "zero being added or subtracted");
}
