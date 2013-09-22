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

	/* we're only const if expr, lhs and rhs are const */
	if(!CONST_AT_COMPILE_TIME(consts[0].type)
	|| !CONST_AT_COMPILE_TIME(consts[1].type)
	|| !CONST_AT_COMPILE_TIME(consts[2].type))
	{
		k->type = CONST_NO;
		return;
	}

	switch(consts[0].type){
		case CONST_NUM:
		{
			numeric *n = &consts[0].bits.num;
			res = n->suffix & VAL_FLOATING ? n->val.f : n->val.i;
			break;
		}
		case CONST_ADDR:
		case CONST_STRK:
			res = 1;
			break;

		case CONST_NEED_ADDR:
		case CONST_NO:
			ICE("buh");
	}

	memcpy_safe(k, &consts[res ? 1 : 2]);
}

void fold_expr_if(expr *e, symtable *stab)
{
	consty konst;
	type_ref *tt_l, *tt_r;

	FOLD_EXPR(e->expr, stab);
	const_fold(e->expr, &konst);

	fold_check_expr(e->expr, FOLD_CHK_NO_ST_UN, "if-expr");

	if(e->lhs){
		FOLD_EXPR(e->lhs, stab);
		fold_check_expr(e->lhs,
				FOLD_CHK_NO_ST_UN | FOLD_CHK_ALLOW_VOID,
				"if-lhs");
	}

	FOLD_EXPR(e->rhs, stab);
	fold_check_expr(e->rhs,
			FOLD_CHK_NO_ST_UN | FOLD_CHK_ALLOW_VOID,
			"if-rhs");


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
		e->tree_type = op_promote_types(
				op_unknown,
				(e->lhs ? &e->lhs : &e->expr), &e->rhs,
				&e->where, stab);

	}else if(type_ref_is_void(tt_l) || type_ref_is_void(tt_r)){
		e->tree_type = type_ref_new_type(type_new_primitive(type_void));

	}else if(type_ref_cmp(tt_l, tt_r, 0) == TYPE_EQUAL){
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
				fold_type_chk_warn(
						tt_l, tt_r, &e->where, "?: pointer type mismatch");

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


basic_blk *gen_expr_if(expr *e, basic_blk *bb)
{
	char *lblfin;

	lblfin = out_label_code("ifexp_fi");

	bb = gen_expr(e->expr, bb);

	if(e->lhs){
		char *lblelse = out_label_code("ifexp_else");

		out_jfalse(bb, lblelse);

		bb = gen_expr(e->lhs, bb);

		out_push_lbl(bb, lblfin, 0);
		out_jmp(bb);

		out_label(bb, lblelse);
		free(lblelse);

	}else{
		out_dup(bb);

		out_jtrue(bb, lblfin);
	}

	out_pop(bb);

	bb = gen_expr(e->rhs, bb);
	out_label(bb, lblfin);

	free(lblfin);

	return bb;
}

basic_blk *gen_expr_str_if(expr *e, basic_blk *bb)
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

	return bb;
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

basic_blk *gen_expr_style_if(expr *e, basic_blk *bb)
{
	bb = gen_expr(e->expr, bb);
	stylef(" ? ");
	if(e->lhs)
		bb = gen_expr(e->lhs, bb);
	stylef(" : ");
	bb = gen_expr(e->rhs, bb);
	return bb;
}
