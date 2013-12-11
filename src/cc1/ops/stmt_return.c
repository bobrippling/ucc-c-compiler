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
		char buf[TYPE_REF_STATIC_BUFSIZ];

		FOLD_EXPR(s->expr, s->symtab);
		fold_need_expr(s->expr, "return", 0);

		if(ret_ty){
			fold_type_ref_equal(ret_ty, s->expr->tree_type,
					&s->where, WARN_RETURN_TYPE, 0,
					"mismatching return type for %s (%s <-- %s)",
					in_func->spel,
					type_ref_to_str_r(buf, ret_ty),
					type_ref_to_str(s->expr->tree_type));

			if(void_func){
				cc1_warn_at(&s->where, 0, WARN_RETURN_TYPE,
						"return with a value in void function %s", in_func->spel);
			}else{
				fold_insert_casts(ret_ty, &s->expr,
						s->symtab, &s->expr->where, "return");
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
		expr_block_set_ty(in_func, ret_ty);
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
