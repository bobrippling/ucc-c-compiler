#include "ops.h"

const char *str_expr_val()
{
	return "val";
}

void gen_expr_val_1(expr *e, FILE *f)
{
	fprintf(f, "%s", asm_intval_str(&e->val.iv));
}

void fold_expr_val(expr *e, symtable *stab)
{
	(void)e;
	(void)stab;
	e->tree_type = decl_new();
	e->tree_type->type->primitive = type_int;
}

void gen_expr_val(expr *e, symtable *stab)
{
	(void)stab;

	if(e->val.iv.suffix)
		ICE("TODO: output with asm intval suffix");

	asm_output_new(
			asm_out_type_mov,
			asm_operand_new_reg(e->tree_type, ASM_REG_A),
			asm_operand_new_val(e->val.iv.val)
		);

	/*
	fputs("\tmov rax, ", cc_out[SECTION_TEXT]);
	e->f_gen_1(e, cc_out[SECTION_TEXT]);
	fputc('\n', cc_out[SECTION_TEXT]);
	*/

	asm_push(ASM_REG_A);
}

void gen_expr_str_val(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("val: %d\n", e->val);
}

expr *expr_new_val(int val)
{
	expr *e = expr_new_wrapper(val);

	e->f_gen_1 = gen_expr_val_1;
	e->val.iv.val = val;

	return e;
}
