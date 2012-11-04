#include <string.h>

#include "ops.h"
#include "expr_val.h"
#include "../out/asm.h"

const char *str_expr_val()
{
	return "val";
}

void fold_expr_val(expr *e, symtable *stab)
{
	(void)stab;

	EOF_WHERE(&e->where,
		e->tree_type = type_ref_new_type(type_new_primitive(type_int)); /* TODO: pull L / U / LU from .val */
	);
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
