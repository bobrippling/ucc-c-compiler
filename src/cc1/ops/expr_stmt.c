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

	last = dynarray_count((void **)e->code->codes);
	if(last){
		last_stmt = e->code->codes[last - 1];
		last_stmt->freestanding = 1; /* allow the final to be freestanding */
	}

	fold_stmt(e->code); /* symtab should've been set by parse */

	if(last && stmt_kind(last_stmt, expr)){
		e->tree_type = decl_copy(last_stmt->expr->tree_type);
	}else{
		e->tree_type = decl_new();
		e->tree_type->type->primitive = type_void; /* void expr */
	}
}

void gen_expr_stmt(expr *e, symtable *stab)
{
	(void)stab;
	gen_stmt(e->code);
	asm_push(ASM_REG_A);
	asm_comment("end of ({...})");
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
