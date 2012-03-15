int exec_op_minus(op)
{
	if(rhs)
		return lhs->val.iv.val - rhs->val.iv.val;
	return -lhs->val.iv.val;
}
