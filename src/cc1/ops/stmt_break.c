#include <string.h>

#include "ops.h"
#include "stmt_break.h"

const char *str_stmt_break()
{
	return "break";
}

void fold_stmt_break_continue(stmt *t, char *lbl)
{
	if(!lbl)
		die_at(&t->where, "%s outside a flow-control statement", t->f_str());

	t->expr = expr_new_identifier(lbl);
	memcpy_safe(&t->expr->where, &t->where);

	t->expr->tree_type = type_ref_cached_VOID();
}

void fold_stmt_break(stmt *t)
{
	fold_stmt_break_continue(t, t->parent ? t->parent->lbl_break : NULL);
}

void gen_stmt_break(stmt *s)
{
	out_push_lbl(s->parent->lbl_break, 0);
	out_jmp();
}

void style_stmt_break(stmt *s)
{
	stylef("break;");
	gen_stmt(s->lhs);
}

void init_stmt_break(stmt *s)
{
	s->f_passable = fold_passable_yes;
}
