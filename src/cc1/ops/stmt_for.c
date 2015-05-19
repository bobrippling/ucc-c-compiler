#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_for.h"
#include "stmt_code.h"
#include "../out/lbl.h"
#include "../decl_init.h"

const char *str_stmt_for()
{
	return "for";
}

void stmt_for_got_decls(stmt *s)
{
	flow_fold(s->flow, &s->symtab);
}

void fold_stmt_for(stmt *s)
{
	if(s->flow->for_while){
		fold_check_expr(
				s->flow->for_while,
				FOLD_CHK_NO_ST_UN | FOLD_CHK_BOOL,
				"for-test");
	}

	fold_stmt(s->lhs);
}

void gen_stmt_for(const stmt *s, out_ctx *octx)
{
	struct out_dbg_lbl *el[2][2];
	out_blk *blk_test = out_blk_new(octx, "for_test"),
	        *blk_body = out_blk_new(octx, "for_body"),
	        *blk_end = out_blk_new(octx, "for_end"),
	        *blk_inc = out_blk_new(octx, "for_inc");

	flow_gen(s->flow, s->flow->for_init_symtab, el, octx);

	/* don't else-if, possible to have both (comma-exp for init) */
	if(s->flow->for_init){
		out_val_consume(octx, gen_expr(s->flow->for_init, octx));

		out_comment(octx, "for-init");
	}

	out_ctrl_transfer_make_current(octx, blk_test);
	if(s->flow->for_while){
		const out_val *for_cond;

		for_cond = gen_expr(s->flow->for_while, octx);

		out_ctrl_branch(octx, for_cond, blk_body, blk_end);
	}else{
		out_ctrl_transfer(octx, blk_body, NULL, NULL);
	}

	stmt_init_blks(s, blk_inc, blk_end);

	out_current_blk(octx, blk_body);
	{
		gen_stmt(s->lhs, octx);
		out_ctrl_transfer(octx, blk_inc, NULL, NULL);
	}

	out_current_blk(octx, blk_inc);
	{
		if(s->flow->for_inc)
			out_val_consume(octx, gen_expr(s->flow->for_inc, octx));
		out_ctrl_transfer(octx, blk_test, NULL, NULL);
	}

	out_current_blk(octx, blk_end);
	flow_end(s->flow, s->flow->for_init_symtab, el, octx);
}

void dump_stmt_for(const stmt *s, dump *ctx)
{
	dump_desc_stmt(ctx, "for", s);

	dump_inc(ctx);

	dump_expr(s->expr, ctx);

	dump_dec(ctx);
}

void style_stmt_for(const stmt *s, out_ctx *octx)
{
	stylef("for(");
	if(s->flow->for_init)
		IGNORE_PRINTGEN(gen_expr(s->flow->for_init, octx));

	stylef("; ");
	if(s->flow->for_while)
		IGNORE_PRINTGEN(gen_expr(s->flow->for_while, octx));

	stylef("; ");
	if(s->flow->for_inc)
		IGNORE_PRINTGEN(gen_expr(s->flow->for_inc, octx));

	stylef(")\n");

	gen_stmt(s->lhs, octx);
}

struct walk_info
{
	stmt *escape;
	int switch_depth;
};

/* ??? change this so we set properties in fold() instead? */
static void
stmt_walk_first_break_goto(stmt *current, int *stop, int *descend, void *extra)
{
	struct walk_info *wi = extra;
	int found = 0;

	(void)descend;

	if(stmt_kind(current, break)){
		found = wi->switch_depth == 0;
	}else if(stmt_kind(current, goto)){
		found = 1;
	}else if(stmt_kind(current, switch)){
		wi->switch_depth++;
	}

	if(found){
		wi->escape = current;
		*stop = 1;
	}
}

static void
stmt_walk_switch_leave(stmt *current, void *extra)
{
	if(stmt_kind(current, switch)){
		struct walk_info *wi = extra;
		wi->switch_depth--;
	}
}

int fold_code_escapable(stmt *s)
{
	struct walk_info wi;

	memset(&wi, 0, sizeof wi);

	stmt_walk(s->lhs, stmt_walk_first_break_goto, stmt_walk_switch_leave, &wi);

	/* we only return if we find a break or goto statement */
	return !!wi.escape;
}

static int for_passable(stmt *s)
{
	/* if we don't have a condition, it's an infinite loop;
	 * check for breaks, etc
	 *
	 * if we have a condition that is constant-non-zero, infinite loop;
	 * check for breaks, etc
	 */

	if(s->flow->for_while){
		if(const_expr_and_non_zero(s->flow->for_while)){
			/* need to check */
		}else{
			return 1; /* non-constant expr or zero-expr - passable */
		}
	}else{
		/* need to check - break */
	}

	return fold_code_escapable(s);
}

void init_stmt_for(stmt *s)
{
	s->f_passable = for_passable;
}
