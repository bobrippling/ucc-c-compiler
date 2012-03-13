#include <string.h>

#include "ops.h"
#include "stmt_default.h"

const char *str_stmt_default()
{
	return "default";
}

void fold_stmt_default(stmt *s)
{
	if(s->expr){
		s->expr->spel = asm_label_case(CASE_CASE, s->expr->val.iv.val);
	}else{
		s->expr = expr_new_identifier(NULL);
		memcpy(&s->expr->where, &s->where, sizeof s->expr->where);

		s->expr->spel = asm_label_case(CASE_DEF, s->expr->val.iv.val);
		s->expr->expr_is_default = 1;
	}

	fold_stmt_and_add_to_curswitch(s);
}

func_gen_stmt (*gen_stmt_default) = gen_stmt_label;
