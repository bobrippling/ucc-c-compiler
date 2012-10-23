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
		char buf[DECL_STATIC_BUFSIZ];

		FOLD_EXPR(s->expr, s->symtab);
		fold_need_expr(s->expr, "return", 0);

		fold_decl_equal(curdecl_func_called, s->expr->tree_type,
				&s->where, WARN_RETURN_TYPE,
				"mismatching return type for %s (%s <-- %s)",
				curdecl_func->spel,
				decl_to_str_r(buf, curdecl_func_called),
				decl_to_str(s->expr->tree_type));

		if(void_func){
			cc1_warn_at(&s->where, 0, 1, WARN_RETURN_TYPE,
					"return with a value in void function %s", curdecl_func->spel);
		}else{
			fold_insert_casts(curdecl_func_called, &s->expr, s->symtab, &s->expr->where, "return");
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
		out_pop_func_ret(s->expr->tree_type);
		out_comment("return");
	}
	out_push_lbl(curfunc_lblfin, 0, NULL);
	out_jmp();
}

void mutate_stmt_return(stmt *s)
{
	s->f_passable = fold_passable_no;
}
