#include "ops.h"
#include "stat_return.h"

const char *str_stat_return()
{
	return "return";
}

void fold_stat_return(stat *s)
{
	if(s->expr){
		fold_expr(s->expr, s->symtab);
		fold_test_expr(s->expr, "return");
	}
}

void gen_stat_return(stat *s)
{
	if(s->expr){
		gen_expr(s->expr, s->symtab);
		asm_temp(1, "pop rax ; return");
	}
	asm_temp(1, "jmp %s", curfunc_lblfin);
}
