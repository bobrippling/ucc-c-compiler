#include "ops.h"
#include "expr_stmt.h"
#include "../../util/dynarray.h"
#include "../type_nav.h"

static const out_val *expr_stmt_lea(expr *, out_ctx *);

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

		if(expr_is_lval(last_expr)){
			e->f_lea = expr_stmt_lea;
			e->lvalue_internal = 1;
		}
	}else{
		e->tree_type = type_nav_btype(cc1_type_nav, type_void);
	}

	e->freestanding = 1; /* ({ ... }) on its own is freestanding */
}

const out_val *gen_expr_stmt(expr *e, out_ctx *octx)
{
	size_t n;
	gen_stmt_code_m1(e->code, 1, octx);

	n = dynarray_count(e->code->bits.code.stmts);

	if(n > 0 && stmt_kind(e->code->bits.code.stmts[n-1], expr))
		return gen_expr(e->code->bits.code.stmts[n - 1]->expr, octx);

	return out_new_noop(octx);
}

static const out_val *expr_stmt_lea(expr *e, out_ctx *octx)
{
	size_t n;

	gen_stmt_code_m1(e->code, 1, octx);

	n = dynarray_count(e->code->bits.code.stmts);
	return lea_expr(e->code->bits.code.stmts[n - 1]->expr, octx);
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
