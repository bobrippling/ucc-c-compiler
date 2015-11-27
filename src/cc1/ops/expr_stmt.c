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
		expr *last_expr = last_stmt->expr;

		e->tree_type = last_expr->tree_type;
		fold_check_expr(e,
				FOLD_CHK_ALLOW_VOID,
				"({ ... }) statement");

		switch(expr_is_lval(last_expr)){
			case LVALUE_NO:
				break;
			case LVALUE_STRUCT:
			case LVALUE_USER_ASSIGNABLE:
				e->f_islval = expr_is_lval_struct;
		}
	}else{
		e->tree_type = type_nav_btype(cc1_type_nav, type_void);
	}

	e->freestanding = 1; /* ({ ... }) on its own is freestanding */
}

static const expr *expr_stmt_last(const expr *e)
{
	size_t n = dynarray_count(e->code->bits.code.stmts);
	stmt *last;

	if(n == 0)
		return NULL;

	last = e->code->bits.code.stmts[n-1];
	if(!stmt_kind(last, expr))
		return NULL;

	return last->expr;
}

const out_val *gen_expr_stmt(const expr *e, out_ctx *octx)
{
	const out_val *ret;
	struct out_dbg_lbl *pushed_lbls[2];
	const expr *last;

	gen_stmt_code_m1(e->code, 1, pushed_lbls, octx);

	last = expr_stmt_last(e);
	ret = (last ? gen_expr(last, octx) : out_new_noop(octx));

	/* this is skipped by gen_stmt_code_m1( ... 1, ... ) */
	gen_stmt_code_m1_finish(e->code, pushed_lbls, octx);

	return ret;
}

irval *gen_ir_expr_stmt(const expr *e, irctx *ctx)
{
	const expr *last;

	gen_ir_stmt_code_m1(e->code, ctx, 1);

	last = expr_stmt_last(e);
	if(!last)
		return irval_from_noop();

	return gen_ir_expr(last, ctx);
}

void dump_expr_stmt(const expr *e, dump *ctx)
{
	dump_desc_expr(ctx, "statement expression", e);
	dump_inc(ctx);
	dump_stmt(e->code, ctx);
	dump_dec(ctx);
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

const out_val *gen_expr_style_stmt(const expr *e, out_ctx *octx)
{
	stylef("({\n");
	gen_stmt(e->code, octx);
	stylef("\n})");
	return NULL;
}
