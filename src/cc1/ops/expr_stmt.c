#include "ops.h"
#include "expr_stmt.h"
#include "../../util/dynarray.h"
#include "../type_nav.h"

const char *str_expr_stmt()
{
	return "statement";
}

void fold_expr_stmt(expr *e, symtable *stab)
{
	stmt *last_stmt;
	int last;

	(void)stab;

	last = dynarray_count(e->code->bits.code.stmts);
	if(last){
		last_stmt = e->code->bits.code.stmts[last - 1];
		last_stmt->freestanding = 1; /* allow the final to be freestanding */
		last_stmt->expr_no_pop = 1;
	}

	fold_stmt(e->code); /* symtab should've been set by parse */

	if(last && stmt_kind(last_stmt, expr)){
		e->tree_type = last_stmt->expr->tree_type;
		fold_check_expr(e,
				FOLD_CHK_ALLOW_VOID | FOLD_CHK_NO_ST_UN,
				"({ ... }) statement");
	}else{
		e->tree_type = type_nav_btype(cc1_type_nav, type_void);
	}

	e->freestanding = 1; /* ({ ... }) on its own is freestanding */
}

void gen_expr_stmt(expr *e)
{
	gen_stmt(e->code);
	/* last stmt is told to leave its result on the stack
	 *
	 * if the last stmt isn't an expression, we put something
	 * on the stack for it
	 */
	{
		int n = dynarray_count(e->code->bits.code.stmts);
		if(n == 0 || !stmt_kind(e->code->bits.code.stmts[n-1], expr))
			out_push_noop();
	}

	out_comment("end of ({...})");
}

void gen_expr_str_stmt(expr *e)
{
	idt_printf("statement:\n");
	gen_str_indent++;
	print_stmt(e->code);
	gen_str_indent--;
}

void mutate_expr_stmt(expr *e)
{
	(void)e;
}

expr *expr_new_stmt(stmt *code)
{
	expr *e = expr_new_wrapper(stmt);
	e->code = code;
	return e;
}

void gen_expr_style_stmt(expr *e)
{
	stylef("({\n");
	gen_stmt(e->code);
	stylef("\n})");
}
