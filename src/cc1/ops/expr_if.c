#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "../sue.h"
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
	fold_disallow_st_un(e->expr, "?: expr");

	if(e->lhs){
		fold_expr(e->lhs, stab);
		fold_disallow_st_un(e->lhs, "?: lhs");
	}

	fold_expr(e->rhs, stab);
	fold_disallow_st_un(e->rhs, "?: rhs");


	/*
	Arithmetic                             Arithmetic                           Arithmetic type after usual arithmetic conversions
	// Structure or union type                Compatible structure or union type   Structure or union type with all the qualifiers on both operands
	void                                   void                                 void
	Pointer to compatible type             Pointer to compatible type           Pointer to type with all the qualifiers specified for the type
	Pointer to type                        NULL pointer (the constant 0)        Pointer to type
	Pointer to object or incomplete type   Pointer to void                      Pointer to void with all the qualifiers specified for the type
	*/

#define tt_l (e->lhs->tree_type)
#define tt_r (e->rhs->tree_type)

	if(decl_is_integral(tt_l) && decl_is_integral(tt_r)){
		ICW("TODO: arithmetic promotions in if_expr");
		/* TODO: arithmetic promotions (with asm_backend_recode) */
		e->tree_type = decl_copy(e->rhs->tree_type);

	}else if(decl_is_void(tt_l) || decl_is_void(tt_r)){
		e->tree_type = decl_new_void();

	}else if(decl_equal(tt_l, tt_r, DECL_CMP_STRICT_PRIMITIVE)){
		e->tree_type = decl_copy(tt_l);

	}else{
		/* brace yourself. */
		int l_ptr_st_un = decl_is_struct_or_union_ptr(tt_l);
		int r_ptr_st_un = decl_is_struct_or_union_ptr(tt_r);

		int l_ptr_null = decl_ptr_depth(tt_l) && const_expr_and_zero(e->lhs);
		int r_ptr_null = decl_ptr_depth(tt_r) && const_expr_and_zero(e->rhs);

		int l_complete = !l_ptr_null && (!l_ptr_st_un || !sue_incomplete(tt_l->type->sue));
		int r_complete = !r_ptr_null && (!r_ptr_st_un || !sue_incomplete(tt_r->type->sue));

		if((l_complete && r_ptr_null) || (r_complete && l_ptr_null)){
			e->tree_type = decl_copy(l_ptr_null ? tt_r : tt_l);

			fprintf(stderr, "l_complete=%d && r_null=%d, r_complete=%d && l_null=%d\n",
				l_complete, r_ptr_null, r_complete, l_ptr_null);

		}else{
			int l_ptr_void = decl_is_void_ptr(tt_l);
			int r_ptr_void = decl_is_void_ptr(tt_r);

			if(l_ptr_void || r_ptr_void){
				e->tree_type = decl_copy(l_ptr_void ? tt_r : tt_l);
			}else{
				char buf[DECL_STATIC_BUFSIZ];

				WARN_AT(&e->where, "conditional type mismatch (%s vs %s)",
						decl_to_str(tt_l), decl_to_str_r(buf, tt_r));
				e->tree_type = decl_new_void();
			}
		}
	}

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

		out_push_lbl(lblfin, 0, NULL);
		out_jmp();

		out_label(lblelse);
		free(lblelse);

	}else{
		out_dup();

		out_jtrue(lblfin);
	}

	out_pop();

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
