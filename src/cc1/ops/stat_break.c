#include "ops.h"
#include "stat_break.h"

const char *str_stat_break()
{
	return "break";
}

void fold_stat_break(stat *t)
{
	if(!curstat_flow)
		die_at(&t->expr->where, "break outside a flow-control statement");

	t->expr = expr_new_identifier(curstat_flow->lblfin);
	t->expr->tree_type = decl_new();
	t->expr->tree_type->type->primitive = type_int;
}

func_gen_stat (*gen_stat_break) = gen_stat_goto;
