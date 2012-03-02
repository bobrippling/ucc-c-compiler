#include "ops.h"
#include "../../util/dynarray.h"

const char *str_expr_stat()
{
	return "statement";
}

void fold_expr_stat(expr *e, symtable *stab)
{
	stat *last_stat;
	int last;

	(void)stab;

	fold_stat(e->code); /* symtab should've been set by parse */

	last = dynarray_count((void **)e->code->codes);
	if(!last)
		die_at(&e->code->where, "no expression in ({ ... })");

	last_stat = e->code->codes[last - 1];

	if(stat_kind(last_stat, expr))
		e->tree_type = decl_copy(last_stat->expr->tree_type);
	else
		e->tree_type->type->primitive = type_void; /* void expr */
}

void gen_expr_stat(expr *e, symtable *stab)
{
	(void)stab;
	gen_stat(e->code);
	asm_temp(1, "push rax ; end of ({...})");
}

void gen_expr_str_stat(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("statement:\n");
	gen_str_indent++;
	print_stat(e->code);
	gen_str_indent--;
}

expr *expr_new_stat(stat *code)
{
	expr *e = expr_new_wrapper(stat);
	e->code = code;
	return e;
}
