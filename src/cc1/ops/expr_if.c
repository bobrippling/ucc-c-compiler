#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "expr_if.h"
#include "../sue.h"
#include "../out/lbl.h"

const char *str_expr_if()
{
	return "if";
}

static void fold_const_expr_if(expr *e, consty *k)
{
	consty consts[3];
	int res;

	const_fold(e->expr, &consts[0]);
	const_fold(e->rhs,  &consts[2]);

	if(e->lhs)
		const_fold(e->lhs, &consts[1]);
	else
		consts[1] = consts[0];

	/* only evaluate lhs/rhs' constness if we need to */
	if(!CONST_AT_COMPILE_TIME(consts[0].type)){
		k->type = CONST_NO;
		return;
	}

	switch(consts[0].type){
		case CONST_VAL:
			res = consts[0].bits.iv.val;
			break;
		case CONST_ADDR:
		case CONST_STRK:
			res = 1;
			break;

		case CONST_NEED_ADDR:
		case CONST_NO:
			ICE("buh");
	}

	res = res ? 1 : 2; /* index into consts */

	if(!CONST_AT_COMPILE_TIME(consts[res].type)){
		k->type = CONST_NO;
	}else{
		memcpy_safe(k, &consts[res]);
		k->nonstandard_const = consts[res == 1 ? 2 : 1].nonstandard_const;
	}
}

void fold_expr_if(expr *e, symtable *stab)
{
	consty konst;
	type_ref *tt_l, *tt_r;

	FOLD_EXPR(e->expr, stab);
	const_fold(e->expr, &konst);

	fold_need_expr(e->expr, "?: expr", 1);
	fold_disallow_st_un(e->expr, "?: expr");

	if(e->lhs){
		FOLD_EXPR(e->lhs, stab);
		fold_disallow_st_un(e->lhs, "?: lhs");
	}

	FOLD_EXPR(e->rhs, stab);
	fold_disallow_st_un(e->rhs, "?: rhs");


	/*

	Arithmetic                             Arithmetic                           Arithmetic type after usual arithmetic conversions
	// Structure or union type                Compatible structure or union type   Structure or union type with all the qualifiers on both operands
	void                                   void                                 void
	Pointer to compatible type             Pointer to compatible type           Pointer to type with all the qualifiers specified for the type
	Pointer to type                        NULL pointer (the constant 0)        Pointer to type
	Pointer to object or incomplete type   Pointer to void                      Pointer to void with all the qualifiers specified for the type

	GCC and Clang seem to relax the last rule:
		a) resolve if either is any pointer, not just (void *)
	  b) resolve to a pointer to the incomplete-type
	*/

	tt_l = (e->lhs ? e->lhs : e->expr)->tree_type;
	tt_r = e->rhs->tree_type;

	if(type_ref_is_integral(tt_l) && type_ref_is_integral(tt_r)){
		expr **middle_op = e->lhs ? &e->lhs : &e->expr;

		expr_check_sign("?:", *middle_op, e->rhs, &e->where);

		e->tree_type = op_promote_types(op_unknown, "?:",
				middle_op, &e->rhs, &e->where, stab);

	}else if(type_ref_is_void(tt_l) || type_ref_is_void(tt_r)){
		e->tree_type = type_ref_new_type(type_new_primitive(type_void));

	}else if(type_ref_equal(tt_l, tt_r, DECL_CMP_EXACT_MATCH)){
		e->tree_type = type_ref_new_cast(tt_l,
				type_ref_qual(tt_l) | type_ref_qual(tt_r));

	}else{
		/* brace yourself. */
		int l_ptr_null = expr_is_null_ptr(e->lhs ? e->lhs : e->expr, 1);
		int r_ptr_null = expr_is_null_ptr(e->rhs, 1);

		int l_complete = !l_ptr_null && type_ref_is_complete(tt_l);
		int r_complete = !r_ptr_null && type_ref_is_complete(tt_r);

		if((l_complete && r_ptr_null) || (r_complete && l_ptr_null)){
			e->tree_type = l_ptr_null ? tt_r : tt_l;

		}else{
			int l_ptr = l_ptr_null || type_ref_is(tt_l, type_ref_ptr);
			int r_ptr = r_ptr_null || type_ref_is(tt_r, type_ref_ptr);

			if(l_ptr || r_ptr){
				char bufa[TYPE_REF_STATIC_BUFSIZ], bufb[TYPE_REF_STATIC_BUFSIZ];

				fold_type_ref_equal(tt_l, tt_r, &e->where,
						WARN_COMPARE_MISMATCH, 0, /* FIXME: enum "mismatch" */
						"pointer type mismatch: %s and %s",
						type_ref_to_str_r(bufa, tt_l),
						type_ref_to_str_r(bufb, tt_r));

				/* void * */
				e->tree_type = type_ref_new_ptr(type_ref_cached_VOID(), qual_none);

				{
					enum type_qualifier q = type_ref_qual(tt_l) | type_ref_qual(tt_r);

					if(q)
						e->tree_type = type_ref_new_cast(e->tree_type, q);
				}

			}else{
				char buf[TYPE_REF_STATIC_BUFSIZ];

				warn_at(&e->where, "conditional type mismatch (%s vs %s)",
						type_ref_to_str(tt_l), type_ref_to_str_r(buf, tt_r));

				e->tree_type = type_ref_cached_VOID();
			}
		}
	}

	e->freestanding = (e->lhs ? e->lhs : e->expr)->freestanding || e->rhs->freestanding;
}


void gen_expr_if(expr *e)
{
	char *lblfin;

	lblfin = out_label_code("ifexp_fi");

	gen_expr(e->expr);

	if(e->lhs){
		char *lblelse = out_label_code("ifexp_else");

		out_jfalse(lblelse);

		gen_expr(e->lhs);

		out_push_lbl(lblfin, 0);
		out_jmp();

		out_label(lblelse);
		free(lblelse);

	}else{
		out_dup();

		out_jtrue(lblfin);
	}

	out_pop();

	gen_expr(e->rhs);
	out_label(lblfin);

	free(lblfin);
}

void gen_expr_str_if(expr *e)
{
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

	gen_str_indent--;
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

void gen_expr_style_if(expr *e)
{
	gen_expr(e->expr);
	stylef(" ? ");
	if(e->lhs)
		gen_expr(e->lhs);
	stylef(" : ");
	gen_expr(e->rhs);
}
