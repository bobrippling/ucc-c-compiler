#include "ops.h"
#include "stat_default.h"

const char *str_stat_default()
{
	return "default";
}

void fold_stat_default(stat *s)
{
	if(s->expr){
		s->expr->spel = asm_label_case(CASE_CASE, s->expr->val.iv.val);
	}else{
		s->expr = expr_new_identifier(NULL);
		s->expr->spel = asm_label_case(CASE_CASE, s->expr->val.iv.val);
		s->expr->expr_is_default = 1;
	}

	fold_stat_and_add_to_curswitch(s);
}

func_gen_stat (*gen_stat_default) = gen_stat_label;
