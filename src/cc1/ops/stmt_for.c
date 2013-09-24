#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_for.h"
#include "stmt_code.h"
#include "../out/lbl.h"
#include "../out/basic_block/bb.h"
#include "../decl_init.h"

const char *str_stmt_for()
{
	return "for";
}

void fold_stmt_for(stmt *s)
{
	symtable *stab = NULL;
	flow_fold(s->flow, &stab);
	UCC_ASSERT(stab, "fold_flow in for didn't pick up .flow");

	s->lbl_break    = out_label_flow("for_start");
	s->lbl_continue = out_label_flow("for_contiune");

#define FOLD_IF(x) if(x) FOLD_EXPR(x, stab)
	FOLD_IF(s->flow->for_init);
	FOLD_IF(s->flow->for_while);
	FOLD_IF(s->flow->for_inc);
#undef FOLD_IF

	if(s->flow->for_while)
		fold_check_expr(
				s->flow->for_while,
				FOLD_CHK_NO_ST_UN | FOLD_CHK_BOOL,
				"for-while");

	fold_stmt(s->lhs);
}

basic_blk *gen_stmt_for(stmt *s, basic_blk *bb)
{
#if 0
	struct basic_blk *b_start, *b_loop, *b_fin;

	bb = flow_gen(s->flow, s->flow->for_init_symtab, bb);

	/* don't else-if, possible to have both (comma-exp for init) */
	if(s->flow->for_init){
		bb = gen_expr(s->flow->for_init, bb);

		out_pop(bb);
		out_comment(bb, "for-init");
	}

	/* start of loop, after init */
	b_start = bb;
	b_loop = bb_new("for_loop"), b_fin = bb_new("for_fin");

	if(s->flow->for_while){
		bb = gen_expr(s->flow->for_while, bb);

		bb_split(bb, b_loop, b_fin, &phi);
	}

	bb = gen_stmt(s->lhs, b_loop);
	if(s->flow->for_inc){
		b_loop = gen_expr(s->flow->for_inc, b_loop);

		out_pop(bb);
		out_comment(bb, "unused for inc");

		bb_link_forward(b_loop, b_start); /* i++, then check */
	}

	return b_fin;
#else
	(void)s, (void)bb;
	ICE("TODO: for");
#endif
}

basic_blk *style_stmt_for(stmt *s, basic_blk *bb)
{
	stylef("for(");
	if(s->flow->for_init)
		bb = gen_expr(s->flow->for_init, bb);

	stylef("; ");
	if(s->flow->for_while)
		bb = gen_expr(s->flow->for_while, bb);

	stylef("; ");
	if(s->flow->for_inc)
		bb = gen_expr(s->flow->for_inc, bb);

	stylef(")\n");

	bb = gen_stmt(s->lhs, bb);

	return bb;
}

struct walk_info
{
	stmt *escape;
	int switch_depth;
};

/* ??? change this so we set properties in fold() instead? */
static void
stmt_walk_first_break_goto_return(stmt *current, int *stop, int *descend, void *extra)
{
	struct walk_info *wi = extra;
	int found = 0;

	(void)descend;

	if(stmt_kind(current, break)){
		found = wi->switch_depth == 0;
	}else if(stmt_kind(current, return) || stmt_kind(current, goto)){
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

	stmt_walk(s->lhs, stmt_walk_first_break_goto_return, stmt_walk_switch_leave, &wi);

	/* we only return if we find a break, goto or return statement */
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

void mutate_stmt_for(stmt *s)
{
	s->f_passable = for_passable;
}
