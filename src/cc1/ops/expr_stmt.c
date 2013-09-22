#include "ops.h"
#include "expr_stmt.h"
#include "../../util/dynarray.h"

const char *str_expr_stmt()
{
	return "statement";
}

void fold_expr_stmt(expr *e, symtable *stab)
{
	stmt *last_stmt;
	int last;

	(void)stab;

	last = dynarray_count(e->code->codes);
	if(last){
		last_stmt = e->code->codes[last - 1];
		last_stmt->freestanding = 1; /* allow the final to be freestanding */
		last_stmt->expr_no_pop = 1;
	}

	fold_stmt(e->code); /* symtab should've been set by parse */

	if(last && stmt_kind(last_stmt, expr)){
		e->tree_type = last_stmt->expr->tree_type;
		fold_check_expr(e, FOLD_CHK_NO_ST_UN, "({ ... }) statement");
	}else{
		e->tree_type = type_ref_cached_VOID(); /* void expr */
	}

	e->freestanding = 1; /* ({ ... }) on its own is freestanding */
}

basic_blk *gen_expr_stmt(expr *e, basic_blk *bb)
{
	bb = gen_stmt(e->code, bb);
	/* last stmt is told to leave its result on the stack
	 *
	 * if the last stmt isn't an expression, we put something
	 * on the stack for it
	 */
	{
		int n = dynarray_count(e->code->codes);
		if(n > 0 && !stmt_kind(e->code->codes[n-1], expr))
			out_push_noop(bb);
	}

	out_comment(bb, "end of ({...})");

	return bb;
}

basic_blk *gen_expr_str_stmt(expr *e, basic_blk *bb)
{
	idt_printf("statement:\n");
	gen_str_indent++;
	print_stmt(e->code);
	gen_str_indent--;

	return bb;
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

basic_blk *gen_expr_style_stmt(expr *e, basic_blk *bb)
{
	stylef("({\n");
	bb = gen_stmt(e->code, bb);
	stylef("\n})");

	return bb;
}
