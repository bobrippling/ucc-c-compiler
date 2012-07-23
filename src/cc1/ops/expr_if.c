#include <stdlib.h>
#include <string.h>

#include "ops.h"

const char *str_expr_if()
{
	return "if";
}

void fold_const_expr_if(expr *e, intval *piv, enum constyness *pconst_type)
{
	intval vals[3];
	enum constyness consts[3];

	const_fold(e->expr, &vals[0], &consts[0]);
	const_fold(e->rhs,  &vals[2], &consts[2]);

	if(e->lhs){
		const_fold(e->lhs, &vals[1], &consts[1]);
	}else{
		consts[1] = consts[0];
		vals[1]   = vals[0];
	}

	*pconst_type = CONST_NO;

	if(consts[0] == CONST_WITH_VAL){
		const int idx_from = vals[0].val ? 1 : 2;

		if(consts[idx_from] == CONST_WITH_VAL){
			memcpy(piv, &vals[idx_from], sizeof *piv);
			*pconst_type = CONST_WITH_VAL;
		}
	}
}

void fold_expr_if(expr *e, symtable *stab)
{
	enum constyness is_const;
	intval dummy;

	fold_expr(e->expr, stab);
	const_fold(e->expr, &dummy, &is_const);

	if(is_const != CONST_NO)
		POSSIBLE_OPT(e->expr, "constant ?: expression");

	fold_test_expr(e->expr, "?: expr");

	if(e->lhs)
		fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);
	e->tree_type = decl_copy(e->rhs->tree_type); /* TODO: check they're the same */

	fold_disallow_st_un(e->lhs, "?: lhs");
	fold_disallow_st_un(e->lhs, "?: rhs");

	e->freestanding = (e->lhs ? e->lhs : e->expr)->freestanding || e->rhs->freestanding;
}


void gen_expr_if(expr *e, symtable *stab)
{
	char *lblfin, *lblelse;

	lblfin = asm_label_code("ifexpa");

	gen_expr(e->expr, stab);

	if(e->lhs){
		lblelse = asm_label_code("ifexpb");

		asm_pop(e->expr->tree_type, ASM_REG_A);
		ASM_TEST(e->expr->tree_type, ASM_REG_A);
		asm_jmp_if_zero(0, lblelse);
		gen_expr(e->lhs, stab);
		asm_jmp(lblfin);
		asm_label(lblelse);
	}else{
		asm_output_new(
				asm_out_type_mov,
				asm_operand_new_reg(e->expr->tree_type, ASM_REG_A),
				asm_operand_new_deref(e->expr->tree_type, asm_operand_new_reg(NULL, ASM_REG_SP), 0)
			);
		asm_comment("save for ?:");

		ASM_TEST(e->expr->tree_type, ASM_REG_A);
		asm_jmp_if_zero(1, lblfin);
		asm_pop(e->expr->tree_type, ASM_REG_A);
		asm_comment("discard lhs");
	}

	gen_expr(e->rhs, stab);
	asm_label(lblfin);

	if(e->lhs)
		free(lblelse);

	free(lblfin);
}

void gen_expr_str_if(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("if expression:\n");
	gen_str_indent++;
#define SUB_PRINT(nam) \
	do{\
		idt_printf(#nam  ":\n"); \
		gen_str_indent++; \
		print_expr(e->nam); \
		gen_str_indent--; \
	}while(0)

	SUB_PRINT(expr);
	if(e->lhs)
		SUB_PRINT(lhs);
	else
		idt_printf("?: syntactic sugar\n");

	SUB_PRINT(rhs);
#undef SUB_PRINT
}

void mutate_expr_if(expr *e)
{
	e->f_const_fold = fold_const_expr_if;
}

expr *expr_new_if(expr *test)
{
	expr *e = expr_new_wrapper(if);
	e->expr = test;
	return e;
}

void gen_expr_style_if(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
