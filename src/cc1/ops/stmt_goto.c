#include <stdlib.h>

#include "ops.h"
#include "stmt_goto.h"

const char *str_stmt_goto()
{
	return "goto";
}

void fold_stmt_goto(stmt *s)
{
	if(s->expr->expr_computed_goto){
		fold_expr(s->expr, s->symtab);
	}else{
		char *save;

		if(!expr_kind(s->expr, identifier))
			DIE_AT(&s->expr->where, "not a label identifier");

		save = s->expr->spel;
		/* else let the assembler check for link errors */
		s->expr->spel = asm_label_goto(s->expr->spel);
		free(save);
	}
}

void gen_stmt_goto(stmt *s)
{
	if(s->expr->expr_computed_goto){
		gen_expr(s->expr, s->symtab);
		asm_pop(NULL, ASM_REG_A);
		asm_output_new(asm_out_type_jmp,
				asm_operand_new_reg(NULL, ASM_REG_A),
				NULL);
	}else{
		asm_jmp(s->expr->spel);
	}
}

void mutate_stmt_goto(stmt *s)
{
	s->f_passable = fold_passable_no;
}
