#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "../out/lbl.h"

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
	char *lblfin;

	lblfin = out_label_code("ifexpa");

	gen_expr(e->expr, stab);

	if(e->lhs){
		char *lblelse = out_label_code("ifexpb");

		out_jfalse(lblelse);

		gen_expr(e->lhs, stab);

		out_push_lbl(lblfin, 0);
		out_jmp();

		out_label(lblelse);
		free(lblelse);

	}else{
		out_dup();

		out_jtrue(lblfin);
	}

	gen_expr(e->rhs, stab);
	out_label(lblfin);

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
