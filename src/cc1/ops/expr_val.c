#include "ops.h"

const char *str_expr_val()
{
	return "val";
}

void gen_expr_val_1(expr *e, FILE *f)
{
	asm_out_intval(f, &e->val.iv);
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

	fputs("\tmov rax, ", cc_out[SECTION_TEXT]);
	e->f_gen_1(e, cc_out[SECTION_TEXT]);
	fputc('\n', cc_out[SECTION_TEXT]);

	asm_temp(1, "push rax");
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
