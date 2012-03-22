optimise_op_multiply(op *o)
{
#define VAL(e, x) (expr_kind(e, val) && e->val.iv.val == x)
	if(op_kind(o, divide))
		goto div;

	if(VAL(e->lhs, 1) || VAL(e->lhs, 0) || (e->rhs ? VAL(e->rhs, 1) || VAL(e->rhs, 0) : 0)){
		POSSIBLE_OPT(e, "1 or 0 being multiplied");
	}else{
div:
		if(VAL(e->rhs, 1))
			POSSIBLE_OPT(e, "divide by 1");
	}
}
