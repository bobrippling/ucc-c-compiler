#include <assert.h>

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
	stmt *last_stmt = NULL;
	size_t last;

	(void)stab;

	last = dynarray_count(e->code->bits.code.stmts);
	if(last){
		last_stmt = e->code->bits.code.stmts[last - 1];
		last_stmt->freestanding = 1; /* allow the final to be freestanding */
		last_stmt->expr_no_pop = 1;
	}

	assert(stmt_kind(e->code, code));
	fold_stmt_code_m1(e->code, 1); /* symtab should've been set by parse */

	if(last_stmt && stmt_kind(last_stmt, expr)){
		expr *last_e = last_stmt->expr;

		fold_expr_no_decay(last_e, last_stmt->symtab);

		/* XXX: statement isn't folded... not bad but probs should do */

		e->is_lval = expr_is_lval(last_e);
		warn_at(&e->where, "lval = %d", e->is_lval);

		e->tree_type = last_e->tree_type;
		/* check _this_ statement expression */
		fold_check_expr(e,
				FOLD_CHK_ALLOW_VOID | FOLD_CHK_NO_ST_UN,
				"({ ... }) statement");

	}else if(last_stmt){
		/* another statement, e.g. ({ if(...){...} }) */
		fold_stmt(last_stmt);
	}

	if(!e->tree_type)
		e->tree_type = type_nav_btype(cc1_type_nav, type_void);

	e->freestanding = 1; /* ({ ... }) on its own is freestanding */
}

const out_val *gen_expr_stmt(expr *e, out_ctx *octx)
{
	size_t n;
	const out_val *ret;

	gen_stmt_code_m1(e->code, 1, octx);

	n = dynarray_count(e->code->bits.code.stmts);

	if(n > 0 && stmt_kind(e->code->bits.code.stmts[n-1], expr))
		ret = gen_expr(e->code->bits.code.stmts[n - 1]->expr, octx);
	else
		ret = out_new_noop(octx);

	/* this is skipped by gen_stmt_code_m1( ... 1, ... ) */
	gen_stmt_code_m1_finish(e->code, octx);

	return ret;
}

const out_val *gen_expr_str_stmt(expr *e, out_ctx *octx)
{
	idt_printf("statement:\n");
	gen_str_indent++;
	print_stmt(e->code);
	gen_str_indent--;
	UNUSED_OCTX();
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

const out_val *gen_expr_style_stmt(expr *e, out_ctx *octx)
{
	stylef("({\n");
	gen_stmt(e->code, octx);
	stylef("\n})");
	return NULL;
}
