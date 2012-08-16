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

	fold_need_expr(e->expr, "?: expr", 1);
	fold_disallow_st_un(e->expr, "?: expr");

	if(e->lhs){
		fold_expr(e->lhs, stab);
		fold_disallow_st_un(e->lhs, "?: lhs");
	}

	fold_expr(e->rhs, stab);
	fold_disallow_st_un(e->rhs, "?: rhs");

	e->tree_type = decl_copy(e->rhs->tree_type); /* TODO: check they're the same */

	e->freestanding = (e->lhs ? e->lhs : e->expr)->freestanding || e->rhs->freestanding;
}


void gen_expr_if(expr *e, symtable *stab)
{
	char *lblfin, *lblelse;

	lblfin = asm_label_code("ifexpa");

	gen_expr(e->expr, stab);

	if(e->lhs){
		lblelse = asm_label_code("ifexpb");

		asm_temp(1, "pop rax");
		asm_temp(1, "test rax, rax");
		asm_temp(1, "jz %s", lblelse);
		gen_expr(e->lhs, stab);
		asm_temp(1, "jmp %s", lblfin);
		asm_label(lblelse);
	}else{
		asm_temp(1, "mov rax, [rsp] ; save for ?:");
		asm_temp(1, "test rax, rax");
		asm_temp(1, "jnz %s", lblfin);
		asm_temp(1, "pop rax ; discard lhs");
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
