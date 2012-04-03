#include <stdlib.h>

#include "ops.h"
#include "stmt_if.h"
#include "stmt_for.h"

const char *str_stmt_if()
{
	return "if";
}

void fold_stmt_if(stmt *s)
{
	symtable *test_symtab;

	if(s->flow){
		/* if(char *x = ...) */
		expr *dinit;

		dinit = fold_for_if_init_decls(s);

		if(!dinit)
			die_at(&s->where, "no initialiser to test in if");

		UCC_ASSERT(!s->expr, "if-expr in c99_ucc if-init mode");

		s->expr = dinit;
		test_symtab = s->flow->for_init_symtab;
	}else{
		test_symtab = s->symtab;
	}

	fold_expr(s->expr, test_symtab);

	fold_test_expr(s->expr, s->f_str());
	OPT_CHECK(s->expr, "constant expression in if/while");

	fold_stmt(s->lhs);
	if(s->rhs)
		fold_stmt(s->rhs);
}

void gen_stmt_if(stmt *s)
{
	char *lbl_else = asm_label_code("else");
	char *lbl_fi   = asm_label_code("fi");

	gen_expr(s->expr, s->symtab);
	asm_temp(1, "pop rax");

	asm_temp(1, "test rax, rax");
	asm_temp(1, "jz %s", lbl_else);
	gen_stmt(s->lhs);
	asm_temp(1, "jmp %s", lbl_fi);
	asm_label(lbl_else);
	if(s->rhs)
		gen_stmt(s->rhs);
	asm_label(lbl_fi);

	free(lbl_else);
	free(lbl_fi);
}
