#include "ops.h"
#include "../../util/dynarray.h"

const char *str_expr_stmt()
{
	return "stmtement";
}

void fold_expr_stmt(expr *e, symtable *stab)
{
	stmt *last_stmt;
	int last;

	(void)stab;

	fold_stmt(e->code); /* symtab should've been set by parse */

	last = dynarray_count((void **)e->code->codes);
	if(!last)
		die_at(&e->code->where, "no expression in ({ ... })");

	last_stmt = e->code->codes[last - 1];

	if(stmt_kind(last_stmt, expr))
		e->tree_type = decl_copy(last_stmt->expr->tree_type);
	else
		e->tree_type->type->primitive = type_void; /* void expr */
}

void gen_expr_stmt(expr *e, symtable *stab)
{
	(void)stab;
	gen_stmt(e->code);
	asm_temp(1, "push rax ; end of ({...})");
}

void gen_expr_str_stmt(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("stmtement:\n");
	gen_str_indent++;
	print_stmt(e->code);
	gen_str_indent--;
}

expr *expr_new_stmt(stmt *code)
{
	expr *e = expr_new_wrapper(stmt);
	e->code = code;
	return e;
}
