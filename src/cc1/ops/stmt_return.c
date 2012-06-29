#include "ops.h"
#include "stmt_return.h"

const char *str_stmt_return()
{
	return "return";
}

void fold_stmt_return(stmt *s)
{
	const int void_func = decl_is_void(curdecl_func_called);

	if(s->expr){
		fold_expr(s->expr, s->symtab);

		fold_test_expr(s->expr, "return");

		fold_decl_equal(s->expr->tree_type, curdecl_func_called,
				&s->where, WARN_RETURN_TYPE,
				"mismatching return type for %s", curdecl_func->spel);

		if(void_func){
			cc1_warn_at(&s->where, 0, 1, WARN_RETURN_TYPE,
					"return with a value in void function %s", curdecl_func->spel);
		}else{
			fold_insert_casts(curdecl_func, &s->expr, s->symtab, &s->expr->where);
		}

	}else if(!void_func){
		cc1_warn_at(&s->where, 0, 1, WARN_RETURN_TYPE,
				"empty return in non-void function %s", curdecl_func->spel);

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

void mutate_stmt_return(stmt *s)
{
	s->f_passable = fold_passable_no;
}
