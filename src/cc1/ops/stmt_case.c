#include "ops.h"
#include "stmt_case.h"

const char *str_stmt_case()
{
	return "case";
}

void fold_stmt_case(stmt *t)
{
	fold_expr(t->expr, t->symtab);

	fold_test_expr(t->expr, "case");

	if(const_fold(t->expr))
		die_at(&t->expr->where, "case expression not constant");

	if(t->expr){
		t->expr->spel = asm_label_case(CASE_CASE, t->expr->val.iv.val);
	}else{
		t->expr = expr_new_identifier(NULL);
		t->expr->spel = asm_label_case(CASE_CASE, t->expr->val.iv.val);
		t->expr->expr_is_default = 1;
	}


	fold_stmt_and_add_to_curswitch(t);
}

func_gen_stmt *gen_stmt_case = gen_stmt_label;
