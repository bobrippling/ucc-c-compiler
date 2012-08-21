#include <string.h>

#include "ops.h"
#include "../out/asm.h"

const char *str_expr_val()
{
	return "val";
}

void gen_expr_val_1(expr *e, FILE *f)
{
	asm_declare_out(f, e->tree_type, "%ld", e->val.iv.val);
}

void fold_expr_val(expr *e, symtable *stab)
{
	(void)e;
	(void)stab;
	eof_where = &e->where;
	e->tree_type = decl_new();
	e->tree_type->type->primitive = type_int;
	eof_where = NULL;
}

void gen_expr_val(expr *e, symtable *stab)
{
	(void)stab;

	out_push_iv(e->tree_type, &e->val.iv);
}

void gen_expr_str_val(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("val: %d\n", e->val);
}

void const_expr_val(expr *e, intval *piv, enum constyness *pconst_type)
{
	memcpy(piv, &e->val, sizeof *piv);
	*pconst_type = CONST_WITH_VAL; /* obviously vals are const */
}

void mutate_expr_val(expr *e)
{
	e->f_gen_1 = gen_expr_val_1;
	e->f_const_fold = const_expr_val;
}

expr *expr_new_val(int val)
{
	expr *e = expr_new_wrapper(val);
	e->val.iv.val = val;
	return e;
}

void gen_expr_style_val(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
