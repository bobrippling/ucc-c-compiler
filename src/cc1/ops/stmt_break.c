#include "ops.h"
#include "stmt_break.h"

const char *str_stmt_break()
{
	return "break";
}

void fold_stmt_break(stmt *t)
{
	if(!curstat_flow)
		die_at(&t->expr->where, "break outside a flow-control stmtement");

	t->expr = expr_new_identifier(curstat_flow->lblfin);
	t->expr->tree_type = decl_new();
	t->expr->tree_type->type->primitive = type_int;
}

func_gen_stmt (*gen_stmt_break) = gen_stmt_goto;
