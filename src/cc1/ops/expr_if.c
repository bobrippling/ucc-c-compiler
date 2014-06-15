#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "expr_if.h"
#include "../sue.h"
#include "../out/lbl.h"
#include "../type_is.h"
#include "../type_nav.h"

const out_val *lea_expr_if(expr *, out_ctx *);

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
	type *tt_l, *tt_r;

	FOLD_EXPR(e->expr, stab);
	const_fold(e->expr, &konst);

	fold_check_expr(e->expr, FOLD_CHK_NO_ST_UN, "if-expr");

	if(e->lhs){
		FOLD_EXPR(e->lhs, stab);
		fold_check_expr(e->lhs,
				FOLD_CHK_ALLOW_VOID,
				"?: left operand");
	}

	FOLD_EXPR(e->rhs, stab);
	fold_check_expr(e->rhs,
			FOLD_CHK_ALLOW_VOID,
			"?: right operand");


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

	if(type_is_arith(tt_l) && type_is_arith(tt_r)){
		expr **middle_op = e->lhs ? &e->lhs : &e->expr;

		expr_check_sign("?:", *middle_op, e->rhs, &e->where);

		e->tree_type = op_promote_types(
				op_unknown,
				middle_op, &e->rhs, &e->where, stab);

	}else if(type_is_void(tt_l) || type_is_void(tt_r)){
		e->tree_type = type_nav_btype(cc1_type_nav, type_void);

	}else if(type_cmp(tt_l, tt_r, 0) & TYPE_EQUAL_ANY){
		/* pointer to 'compatible' type */
		e->tree_type = type_qualify(tt_l,
				type_qual(tt_l) | type_qual(tt_r));

		if(type_is_s_or_u(tt_l)){
			e->f_lea = lea_expr_if;
			e->lvalue_internal = 1;
		}

	}else{
		switch(type_cmp(tt_l, tt_r, 0)){
			case TYPE_EQUAL:
			case TYPE_EQUAL_TYPEDEF:
				/* pointer to 'compatible' type */
				e->tree_type = type_qualify(tt_l,
						type_qual(tt_l) | type_qual(tt_r));
				break;

			case TYPE_QUAL_POINTED_ADD:
			case TYPE_QUAL_POINTED_SUB:
			{
				/* difference in nested qualifiers - combine from both */
				e->tree_type =
					type_ptr_to(
							type_qualify(
								type_next(tt_l),
								type_qual(type_next(tt_r))));
				break;
			}

			case TYPE_QUAL_ADD:
			case TYPE_QUAL_SUB:
			{
				/* difference only in qualifiers - combine from both */
				e->tree_type = type_qualify(
						type_unqualify(tt_l),
						type_qual(tt_l) | type_qual(tt_r));
				break;
			}

			default:
			{
				/* brace yourself. */
				int l_ptr_null = expr_is_null_ptr(
						e->lhs ? e->lhs : e->expr, NULL_STRICT_INT);

				int r_ptr_null = expr_is_null_ptr(e->rhs, NULL_STRICT_INT);

				int l_complete = !l_ptr_null && type_is_complete(tt_l);
				int r_complete = !r_ptr_null && type_is_complete(tt_r);

				if((l_complete && r_ptr_null) || (r_complete && l_ptr_null)){
					e->tree_type = l_ptr_null ? tt_r : tt_l;

				}else{
					int l_strict_ptr = !!type_is(tt_l, type_ptr);
					int r_strict_ptr = !!type_is(tt_r, type_ptr);
					int l_ptr = l_ptr_null || l_strict_ptr;
					int r_ptr = r_ptr_null || r_strict_ptr;

					if(l_ptr || r_ptr){
						enum type_qualifier qual_both
							= (l_strict_ptr ? type_qual(type_next(tt_l)) : qual_none)
							|
							(r_strict_ptr ? type_qual(type_next(tt_r)) : qual_none);

						fold_type_chk_warn(
								tt_l, tt_r, &e->where, "?: pointer type mismatch");

						/* qualified void * */
						e->tree_type = type_ptr_to(
								type_qualify(
									type_nav_btype(cc1_type_nav, type_void),
									qual_both));

					}else{
						char buf[TYPE_STATIC_BUFSIZ];

						warn_at(&e->where, "conditional type mismatch (%s vs %s)",
								type_to_str(tt_l), type_to_str_r(buf, tt_r));

						e->tree_type = type_nav_btype(cc1_type_nav, type_void);
					}
				}
			}
		}
	}

	e->freestanding = (e->lhs ? e->lhs : e->expr)->freestanding || e->rhs->freestanding;
}

static const out_val *gen_expr_if_via(
		expr *e, out_ctx *octx,
		const out_val *fn(expr *, out_ctx *))
{
	out_blk *landing = out_blk_new(octx, "if_end"),
	        *blk_lhs = out_blk_new(octx, "if_lhs"),
	        *blk_rhs = out_blk_new(octx, "if_rhs");
	const out_val *cond = gen_expr(e->expr, octx);

	if(!e->lhs)
		out_val_retain(octx, cond);

	out_ctrl_branch(octx, cond, blk_lhs, blk_rhs);

	out_current_blk(octx, blk_lhs);
	{
		out_ctrl_transfer(octx, landing,
				e->lhs ? fn(e->lhs, octx) : cond,
				&blk_lhs);
	}

	out_current_blk(octx, blk_rhs);
	{
		out_ctrl_transfer(octx, landing,
				gen_expr(e->rhs, octx), &blk_rhs);
	}

	out_current_blk(octx, landing);
	return out_ctrl_merge(octx, blk_lhs, blk_rhs);
}

const out_val *gen_expr_if(expr *e, out_ctx *octx)
{
	return gen_expr_if_via(e, octx, gen_expr);
}

const out_val *lea_expr_if(expr *e, out_ctx *octx)
{
	return gen_expr_if_via(e, octx, lea_expr);
}

const out_val *gen_expr_str_if(expr *e, out_ctx *octx)
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

	UNUSED_OCTX();
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

const out_val *gen_expr_style_if(expr *e, out_ctx *octx)
{
	IGNORE_PRINTGEN(gen_expr(e->expr, octx));
	stylef(" ? ");
	if(e->lhs)
		IGNORE_PRINTGEN(gen_expr(e->lhs, octx));
	stylef(" : ");
	IGNORE_PRINTGEN(gen_expr(e->rhs, octx));
	return NULL;
}
