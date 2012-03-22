gen_op_divide(op *op, symtable *stab)
{
	/*
	 * idiv Integer Division
	 * divide the contents of the 64 bit integer EDX:EAX by the operand
	 * quotient = eax, remainder = edx
	 */

	gen_expr(op->lhs, stab);
	gen_expr(op->rhs, stab);
	/* pop top stack (rhs) into b, and next top into a */

	asm_temp(1, "xor rdx,rdx");
	asm_temp(1, "pop rbx");
	asm_temp(1, "pop rax");
	asm_temp(1, "idiv rbx");

	asm_temp(1, "push r%cx", e->op == op_divide ? 'a' : 'd');
}

int exec_op_divide(op)
{
	if(rhs->val.iv.val)
		return lhs->val.iv.val / rhs->val.iv.val;
	warn_at(&rhs->where, "division by zero");
	*bad = 1;
	return 0;
}

optimise_op_divide(op *o)
{
	return optimise_op_multiply(o);
}
