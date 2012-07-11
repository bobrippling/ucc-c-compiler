#include <stdlib.h>
#include "ops.h"
#include "../sue.h"

const char *str_expr_if()
{
	return "if";
}

int fold_const_expr_if(expr *e)
{
	if(!const_fold(e->expr) && (e->lhs ? !const_fold(e->lhs) : 1) && !const_fold(e->rhs)){
		expr_mutate_wrapper(e, val);

		e->val.iv.val =
			e->expr->val.iv.val
			? (e->lhs ? e->lhs->val.iv.val : e->expr->val.iv.val)
			: e->rhs->val.iv.val;

		return 0;
	}
	return 1;
}

void fold_expr_if(expr *e, symtable *stab)
{
	decl *tt_l, *tt_r;

	fold_expr(e->expr, stab);
	if(const_expr_is_const(e->expr))
		POSSIBLE_OPT(e->expr, "constant ?: expression");

	fold_test_expr(e->expr, "?: expr");

	if(e->lhs)
		fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);


	/*
	Arithmetic                             Arithmetic                           Arithmetic type after usual arithmetic conversions
	// Structure or union type                Compatible structure or union type   Structure or union type with all the qualifiers on both operands
	void                                   void                                 void
	Pointer to compatible type             Pointer to compatible type           Pointer to type with all the qualifiers specified for the type
	Pointer to type                        NULL pointer (the constant 0)        Pointer to type
	Pointer to object or incomplete type   Pointer to void                      Pointer to void with all the qualifiers specified for the type
	*/

	tt_l = e->lhs->tree_type;
	tt_r = e->rhs->tree_type;

	if(decl_is_integral(tt_l) && decl_is_integral(tt_r)){
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

		int l_ptr_null = decl_ptr_depth(tt_l) && const_expr_is_zero(e->lhs);
		int r_ptr_null = decl_ptr_depth(tt_r) && const_expr_is_zero(e->rhs);

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
