#include "ops.h"
#include "expr_stmt.h"
#include "../../util/dynarray.h"
#include "../type_nav.h"

const char *str_expr_stmt(void)
{
	return "statement";
}

static expr *find_last_from_stmts(struct stmt_and_decl **entries)
{
	size_t n = dynarray_count(entries);
	size_t lastindex;
	stmt *last_stmt;

	if(n == 0)
		return NULL;

	lastindex = n - 1;
	for(;;){
		struct stmt_and_decl *entry = entries[lastindex];
		last_stmt = entry->stmt;
		if(last_stmt)
			break;
		if(lastindex == 0)
			return NULL;
		lastindex--;
	}

	if(stmt_kind(last_stmt, expr)){
		last_stmt->freestanding = 1; /* allow the final to be freestanding */
		return last_stmt->expr;
	}
	return NULL;
}

static expr *find_last_from_expr(expr *e)
{
	return find_last_from_stmts(e->code->bits.stmt_and_decls);
}

void fold_expr_stmt(expr *e, symtable *stab)
{
	expr *last_expr;

	(void)stab;

	/* do this first, so we can make the expression free-standing before it's type-checked+etc */
	last_expr = find_last_from_expr(e);

	fold_stmt(e->code); /* symtab should've been set by parse */

	if(last_expr){
		e->tree_type = last_expr->tree_type;
		if(fold_check_expr(e,
				FOLD_CHK_ALLOW_VOID,
				"({ ... }) statement"))
		{
			return;
		}

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

const out_val *gen_expr_stmt(const expr *e, out_ctx *octx)
{
	size_t n;
	const out_val *ret = NULL;
	struct out_dbg_lbl *pushed_lbls[2];
	struct stmt_and_decl **stmt_and_decls = e->code->bits.stmt_and_decls;

	gen_stmt_code_m1(e->code, 1, pushed_lbls, octx);

	n = dynarray_count(stmt_and_decls);
	if(n > 0){
		struct stmt_and_decl *last = stmt_and_decls[n-1];

		if(last->stmt){
			if(stmt_kind(last->stmt, expr))
				ret = gen_expr(last->stmt->expr, octx);
			else
				gen_stmt(last->stmt, octx);
		}
	}

	if(!ret)
		ret = out_new_noop(octx);

	/* this is skipped by gen_stmt_code_m1( ... 1, ... ) */
	gen_stmt_code_m1_finish(e->code, pushed_lbls, octx);

	return ret;
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
	e->f_has_sideeffects = expr_bool_always;
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
