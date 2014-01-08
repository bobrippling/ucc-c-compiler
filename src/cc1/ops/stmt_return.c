#include "ops.h"
#include "stmt_return.h"

#include "expr_block.h"
#include "../funcargs.h"

const char *str_stmt_return()
{
	return "return";
}

void fold_stmt_return(stmt *s)
{
	decl *in_func = symtab_func(s->symtab);
	type_ref *ret_ty;
	int void_func;

	if(!in_func)
		die_at(&s->where, "return outside a function");

	if(in_func->ref){
		ret_ty = type_ref_func_call(in_func->ref, NULL);
		void_func = type_ref_is_void(ret_ty);
	}else{
		/* we're the first return stmt in a block */
		ret_ty = NULL;
	}

	if(s->expr){
		FOLD_EXPR(s->expr, s->symtab);
		fold_check_expr(s->expr, 0, s->f_str());

		if(ret_ty){
			/* void return handled implicitly with a cast to void */
			fold_type_chk_and_cast(
					ret_ty, &s->expr,
					s->symtab, &s->where, "return type");

			if(void_func){
				cc1_warn_at(&s->where, 0, WARN_RETURN_TYPE,
						"return with a value in void function %s", in_func->spel);
			}
		}else{
			/* in a block */
			void_func = 0;
		}

	}else{
		if(!ret_ty){
			/* in a void block */
			void_func = 1;
		}else if(!void_func){
			cc1_warn_at(&s->where, 0, WARN_RETURN_TYPE,
					"empty return in non-void function %s", in_func->spel);
		}
	}

	if(!ret_ty){
		/* first return of a block */
		ret_ty = s->expr ? s->expr->tree_type : type_ref_cached_VOID();
		expr_block_set_ty(in_func, ret_ty, s->symtab);
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
