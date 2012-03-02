#include <stdlib.h>

#include "ops.h"
#include "stat_for.h"

const char *str_stat_for()
{
	return "for";
}

void fold_stat_for(stat *s)
{
	stat *oldflowstat = curstat_flow;
	curstat_flow = s;

	s->lblfin = asm_label_flowfin();

#define FOLD_IF(x) if(x) fold_expr(x, s->symtab)
	FOLD_IF(s->flow->for_init);
	FOLD_IF(s->flow->for_while);
	FOLD_IF(s->flow->for_inc);
#undef FOLD_IF

	if(s->flow->for_while)
		fold_test_expr(s->flow->for_while, "for-while");

	OPT_CHECK(s->flow->for_while, "constant expression in for");

	fold_stat(s->lhs);

	curstat_flow = oldflowstat;
}

void gen_stat_for(stat *s)
{
	char *lbl_for;

	lbl_for = asm_label_code("for");

	if(s->flow->for_init){
		gen_expr(s->flow->for_init, s->symtab);
		asm_temp(1, "pop rax ; unused for init");
	}

	asm_label(lbl_for);
	if(s->flow->for_while){
		gen_expr(s->flow->for_while, s->symtab);

		asm_temp(1, "pop rax");
		asm_temp(1, "test rax, rax");
		asm_temp(1, "jz %s", s->lblfin);
	}

	gen_stat(s->lhs);
	if(s->flow->for_inc){
		gen_expr(s->flow->for_inc, s->symtab);
		asm_temp(1, "pop rax ; unused for inc");
	}

	asm_temp(1, "jmp %s", lbl_for);

	asm_label(s->lblfin);

	free(lbl_for);
}
