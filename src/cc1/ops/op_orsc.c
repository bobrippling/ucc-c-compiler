optimise_op_andsc(op *o)
{
	/* check if one side is (&& ? false : true) and short circuit it without needing to check the other side */
	if(expr_kind(e->lhs, val) || expr_kind(e->rhs, val))
		POSSIBLE_OPT(e, "short circuit const");
}

static void asm_shortcircuit(expr *e, symtable *tab)
{
	char *baillabel = asm_label_code("shortcircuit_bail");
	gen_expr(e->lhs, tab);

	asm_temp(1, "mov rax,[rsp]");
	asm_temp(1, "test rax,rax");
	/* leave the result on the stack (if false) and bail */
	asm_temp(1, "j%sz %s", e->op == op_andsc ? "" : "n", baillabel);
	asm_temp(1, "pop rax");
	gen_expr(e->rhs, tab);

	asm_label(baillabel);
	free(baillabel);
}

