#include "ops.h"
#include "stmt_return.h"

const char *str_stmt_return()
{
	return "return";
}

void fold_stmt_return(stmt *s)
{
	const int void_func = type_ref_is_void(curdecl_ref_func_called);

	if(s->expr){
		FOLD_EXPR(s->expr, s->symtab);
		fold_check_expr(s->expr, 0, s->f_str());

		/* void return handled implicitly with a cast to void */
		fold_type_chk_and_cast(
				curdecl_ref_func_called, &s->expr,
				s->symtab, &s->where, "return type");

		if(type_ref_is_void(curdecl_ref_func_called)){
			cc1_warn_at(&s->where, 0, WARN_RETURN_TYPE,
					"return with a value in void function %s",
					curdecl_func->spel);
		}

	}else if(!void_func){
		cc1_warn_at(&s->where, 0, WARN_RETURN_TYPE,
				"empty return in non-void function %s",
				curdecl_func->spel);

	}
}

void gen_stmt_return(stmt *s)
{
	if(s->expr){
		gen_expr(s->expr);
		out_pop_func_ret(s->expr->tree_type);
		out_comment("return");
	}
	out_push_lbl(curfunc_lblfin, 0);
	out_jmp();
}

void style_stmt_return(stmt *s)
{
	stylef("return ");
	gen_expr(s->expr);
	stylef(";");
}

void init_stmt_return(stmt *s)
{
	s->f_passable = fold_passable_no;
}
