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

		fold_decl_equal(s->expr->tree_type, curdecl_func_called,
				&s->where, WARN_RETURN_TYPE,
				"mismatching return type for %s", curdecl_func->spel);

	}else if(!decl_is_void(curdecl_func)){
		cc1_warn_at(&s->where, 0, WARN_RETURN_TYPE,
				"empty return in non-void function %s", curdecl_func->spel);
	}
}

void gen_stmt_return(stmt *s)
{
	if(s->expr){
		gen_expr(s->expr, s->symtab);
		asm_temp(1, "pop rax ; return");
	}
	asm_temp(1, "jmp %s", curfunc_lblfin);
}
