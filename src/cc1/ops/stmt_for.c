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

void gen_stmt_for(stmt *s)
{
	char *lbl_test = out_label_flow("for_test");

	flow_gen(s->flow, s->flow->for_init_symtab);

	/* don't else-if, possible to have both (comma-exp for init) */
	if(s->flow->for_init){
		gen_expr(s->flow->for_init);

		out_pop();
		out_comment("for-init");
	}

	out_label(lbl_test);
	if(s->flow->for_while){
		gen_expr(s->flow->for_while);
		out_jfalse(s->lbl_break);
	}

	gen_stmt(s->lhs);
	out_label(s->lbl_continue);
	if(s->flow->for_inc){
		gen_expr(s->flow->for_inc);

		out_pop();
		out_comment("unused for inc");
	}

	out_push_lbl(lbl_test, 0);
	out_jmp();

	out_label(s->lbl_break);

	free(lbl_test);
}

void style_stmt_for(stmt *s)
{
	stylef("for(");
	if(s->flow->for_init)
		gen_expr(s->flow->for_init);

	stylef("; ");
	if(s->flow->for_while)
		gen_expr(s->flow->for_while);

	stylef("; ");
	if(s->flow->for_inc)
		gen_expr(s->flow->for_inc);

	stylef(")\n");

	gen_stmt(s->lhs);
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

void init_stmt_for(stmt *s)
{
	s->f_passable = for_passable;
}
