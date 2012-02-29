
void gen_expr_val(expr *e, symtable *stab)
{
	/*asm_new(asm_load_val, &e->val);*/
	/*asm_temp(1, "mov rax, %d", e->val);*/
	fputs("\tmov rax, ", cc_out[SECTION_TEXT]);
	asm_out_intval(cc_out[SECTION_TEXT], &e->val.i);
	fputc('\n', cc_out[SECTION_TEXT]);
	asm_temp(1, "push rax");
}
