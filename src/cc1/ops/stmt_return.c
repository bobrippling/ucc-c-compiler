#include "ops.h"
#include "stmt_return.h"

const char *str_stmt_return()
{
	return "return";
}

void fold_stmt_return(stmt *s)
{
	if(s->expr){
		fold_expr(s->expr, s->symtab);

		fold_test_expr(s->expr, "return");

		fold_decl_equal(curdecl_func, s->expr->tree_type,
			&s->where, WARN_RET_MISMATCH,
			"type mismatch in return (%s)",
			curdecl_func->spel);

		fold_typecheck_primitive(curdecl_func, &s->expr, s->symtab);
	}
}

void gen_stmt_return(stmt *s)
{
	if(s->expr){
		gen_expr(s->expr, s->symtab);
		asm_pop(curdecl_func, ASM_REG_A);
		asm_comment("return");
	}
	asm_jmp(curfunc_lblfin);
}
