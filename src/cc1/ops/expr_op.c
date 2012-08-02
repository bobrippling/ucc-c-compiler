#include <string.h>
#include <stdlib.h>
#include "ops.h"
#include "../sue.h"
#include "expr_op.h"

const char *str_expr_op()
{
	return "op";
}

void operate(expr *lhs, expr *rhs, enum op_type op, intval *piv, enum constyness *pconst_type)
{
#define OP(a, b) case a: piv->val = lhs->val.iv.val b rhs->val.iv.val; return
	switch(op){
		OP(op_multiply,   *);
		OP(op_eq,         ==);
		OP(op_ne,         !=);
		OP(op_le,         <=);
		OP(op_lt,         <);
		OP(op_ge,         >=);
		OP(op_gt,         >);
		OP(op_xor,        ^);
		OP(op_or,         |);
		OP(op_and,        &);
		OP(op_orsc,       ||);
		OP(op_andsc,      &&);
		OP(op_shiftl,     <<);
		OP(op_shiftr,     >>);

		case op_modulus:
		case op_divide:
		{
			long l = lhs->val.iv.val, r = rhs->val.iv.val;

			if(r){
				piv->val = op == op_divide ? l / r : l % r;
				return;
			}
			warn_at(&rhs->where, 1, "division by zero");
			*pconst_type = CONST_NO;
			return;
		}

		case op_plus:
			piv->val = lhs->val.iv.val + (rhs ? rhs->val.iv.val : 0);
			return;

		case op_minus:
			piv->val = rhs ? lhs->val.iv.val - rhs->val.iv.val : -lhs->val.iv.val;
			return;

		case op_not:  piv->val = !lhs->val.iv.val; return;
		case op_bnot: piv->val = ~lhs->val.iv.val; return;

		case op_deref:
			/*
			 * potential for major ICE here
			 * I mean, we might not even be dereferencing the right size pointer
			 */
			/*
			switch(lhs->vartype->primitive){
				case type_int:  return *(int *)lhs->val.s;
				case type_char: return *       lhs->val.s;
				default:
					break;
			}

			ignore for now, just deal with simple stuff
			*/
			if(lhs->ptr_safe && expr_kind(lhs, addr)){
				if(lhs->array_store->type == array_str){
					/*piv->val = lhs->val.exprs[0]->val.i;*/
					piv->val = *lhs->val.s;
					return;
				}
			}
			*pconst_type = CONST_NO;
			return;

		case op_struct_ptr:
		case op_struct_dot:
			*pconst_type = CONST_NO;
			return;

		case op_unknown:
			break;
	}

	ICE("unhandled operate type");
}

void operate_optimise(expr *e)
{
	/* TODO */

	switch(e->op){
		case op_orsc:
		case op_andsc:
			/* check if one side is (&& ? false : true) and short circuit it without needing to check the other side */
			if(expr_kind(e->lhs, val) || expr_kind(e->rhs, val))
				POSSIBLE_OPT(e, "short circuit const");
			break;

#define VAL(e, x) (expr_kind(e, val) && e->val.iv.val == x)

		case op_plus:
		case op_minus:
			if(VAL(e->lhs, 0) || (e->rhs ? VAL(e->rhs, 0) : 0))
				POSSIBLE_OPT(e, "zero being added or subtracted");
			break;

		case op_multiply:
			if(VAL(e->lhs, 1) || VAL(e->lhs, 0) || (e->rhs ? VAL(e->rhs, 1) || VAL(e->rhs, 0) : 0))
				POSSIBLE_OPT(e, "1 or 0 being multiplied");
			else
		case op_divide:
			if(VAL(e->rhs, 1))
				POSSIBLE_OPT(e, "divide by 1");
			break;

		default:
			break;
#undef VAL
	}
}

void fold_const_expr_op(expr *e, intval *piv, enum constyness *pconst_type)
{
	intval lhs, rhs;
	enum constyness l_const, r_const;

	const_fold(e->lhs, &lhs, &l_const);
	if(e->rhs){
		const_fold(e->rhs, &rhs, &r_const);
	}else{
		r_const = CONST_WITH_VAL;
	}

	if(l_const == CONST_WITH_VAL && r_const == CONST_WITH_VAL){
		*pconst_type = CONST_WITH_VAL;
		operate(e->lhs, e->rhs, e->op, piv, pconst_type);
	}else{
		*pconst_type = CONST_WITHOUT_VAL;
		operate_optimise(e);
	}
}

void fold_op_struct(expr *e, symtable *stab)
{
	/*
	 * lhs = any ptr-to-struct expr
	 * rhs = struct member ident
	 */
	const int ptr_depth_exp = e->op == op_struct_ptr ? 1 : 0;
	struct_union_enum_st *sue;
	char *spel;

	fold_expr(e->lhs, stab);
	/* don't fold the rhs - just a member name */

	if(!expr_kind(e->rhs, identifier))
		DIE_AT(&e->rhs->where, "struct member must be an identifier (got %s)", e->rhs->f_str());
	spel = e->rhs->spel;

	/* we access a struct, of the right ptr depth */
	if(!decl_is_struct_or_union_possible_ptr(e->lhs->tree_type)
	|| decl_ptr_depth(e->lhs->tree_type) != ptr_depth_exp)
	{
		const int ident = expr_kind(e->lhs, identifier);

		DIE_AT(&e->lhs->where, "%s%s%s is not a %sstruct or union (member %s)",
				decl_to_str(e->lhs->tree_type),
				ident ? " " : "",
				ident ? e->lhs->spel : "",
				ptr_depth_exp == 1 ? "pointer-to-" : "",
				spel);
	}

	sue = e->lhs->tree_type->type->sue;

	if(sue_incomplete(sue))
		DIE_AT(&e->lhs->where, "%s incomplete type (%s)",
				ptr_depth_exp == 1
				? "dereferencing pointer to"
				: "use of",
				type_to_str(e->lhs->tree_type->type));

	/* found the struct, find the member */
	e->rhs->tree_type = struct_union_member_find(sue, spel, &e->where);

	/*
	 * if it's a.b, convert to (&a)->b for asm gen
	 * e = { lhs = "a", rhs = "b", type = dot }
	 * e = { lhs = { expr = "a", type = addr }, rhs = "b", type = ptr }
	 */
	if(ptr_depth_exp == 0){
		expr *new;

		eof_where = &e->where;
		new = expr_new_wrapper(addr);
		eof_where = NULL;

		new->lhs = e->lhs;
		e->lhs = new;

		expr_mutate_wrapper(e, op);
		e->op   = op_struct_ptr;

		fold_expr(e->lhs, stab);
	}

	e->tree_type = decl_copy(e->rhs->tree_type);
	/* pull qualifiers from the struct to the member */
	e->tree_type->type->qual |= e->lhs->tree_type->type->qual;
}

void fold_deref(expr *e)
{
	if(decl_attr_present(op_deref_expr(e)->tree_type->attr, attr_noderef))
		WARN_AT(&op_deref_expr(e)->where, "dereference of noderef expression");

	/* check for *&x */
	if(expr_kind(op_deref_expr(e), addr))
		WARN_AT(&op_deref_expr(e)->where, "possible optimisation for *& expression");

	e->tree_type = decl_ptr_depth_dec(decl_copy(op_deref_expr(e)->tree_type), &e->where);

	if(decl_desc_depth(e->tree_type) == 0 && e->tree_type->type->primitive == type_void)
		DIE_AT(&e->where, "can't dereference void pointer (%s)", decl_to_str(e->tree_type));
}

void fold_expr_op(expr *e, symtable *stab)
{
#define IS_PTR(x) decl_ptr_depth(x->tree_type)

#define SPEL_IF_IDENT(hs)                            \
					expr_kind(hs, identifier) ? " ("     : "", \
					expr_kind(hs, identifier) ? hs->spel : "", \
					expr_kind(hs, identifier) ? ")"      : ""  \

#define RHS e->rhs
#define LHS e->lhs

	UCC_ASSERT(e->op != op_unknown, "unknown op in expression at %s",
			where_str(&e->where));

	if(e->op == op_struct_ptr || e->op == op_struct_dot){
		fold_op_struct(e, stab);
		return;
	}

	fold_expr(e->lhs, stab);
	fold_disallow_st_un(e->lhs, "op-lhs");

	if(e->rhs){
		fold_expr(e->rhs, stab);
		fold_disallow_st_un(e->rhs, "op-rhs");

		fold_insert_casts(e->lhs->tree_type, &e->rhs, stab, &e->where, "operation");

		if(decl_is_void(e->lhs->tree_type) || decl_is_void(e->rhs->tree_type))
			DIE_AT(&e->where, "use of void expression");

		fold_decl_equal(e->lhs->tree_type, e->rhs->tree_type,
				&e->where, WARN_COMPARE_MISMATCH,
				"operation between mismatching types%s%s%s%s%s%s",
				SPEL_IF_IDENT(e->lhs), SPEL_IF_IDENT(e->rhs));

		if(op_is_cmp(e->op) && e->lhs->tree_type->type->is_signed != e->rhs->tree_type->type->is_signed){

			/*
			 * assert(LHS == UNSIGNED);
			 * vals default to signed, change to unsigned
			 */

			if(expr_kind(RHS, val) && RHS->val.iv.val >= 0){
				UCC_ASSERT(!e->lhs->tree_type->type->is_signed, "signed-unsigned assumption failure");
				RHS->tree_type->type->is_signed = 0;
			}else if(expr_kind(LHS, val) && LHS->val.iv.val >= 0){
				UCC_ASSERT(!e->rhs->tree_type->type->is_signed, "signed-unsigned assumption failure");
				LHS->tree_type->type->is_signed = 0;
			}else{
					cc1_warn_at(&e->where, 0, 1, WARN_SIGN_COMPARE, "comparison between signed and unsigned%s%s%s%s%s%s",
							SPEL_IF_IDENT(LHS), SPEL_IF_IDENT(RHS));
			}
		}
	}

	if(e->op == op_deref){
		fold_deref(e);
	}else{
		/*
		 * look either side - if either is a pointer, take that as the tree_type
		 *
		 * operation between two values of any type
		 *
		 * TODO: checks for pointer + pointer (invalid), etc etc
		 */

		if(e->rhs){
			if(IS_PTR(e->rhs)){
				e->tree_type = decl_copy(e->rhs->tree_type);
			}else{
				goto norm_tt;
			}
		}else{
norm_tt:
			/* if we have a _comparison_ (e.g. between enums), convert to int */

			if(op_is_cmp(e->op)){
				e->tree_type = decl_new();
				e->tree_type->type->primitive = type_int;
			}else{
				e->tree_type = decl_copy(e->lhs->tree_type);
			}
		}
	}

	if(e->rhs){

		/* need to do this check _after_ we get the correct tree type */
		if((e->op == op_plus || e->op == op_minus)
		&& decl_ptr_depth(e->tree_type)
		&& !e->op_no_ptr_mul)
		{
			/* 2 + (void *)5 is 7, not 2 + 8*5 */
			if(decl_is_void_ptr(e->tree_type)){
				cc1_warn_at(&e->tree_type->type->where, 0, 1, WARN_VOID_ARITH, "arithmetic with void pointer");
			}else{
				/*
				 * subtracting two pointers - need to divide by sizeof(typeof(*expr))
				 * adding int to a pointer - need to multiply by sizeof(typeof(*expr))
				 */

				if(e->op == op_minus){
					/* need to apply the divide to the current 'e' */
					expr *sub = expr_new_op(e->op);
					decl *const cpy = decl_ptr_depth_dec(decl_copy(e->tree_type), &e->where);

					memcpy(&sub->where, &e->where, sizeof sub->where);

					sub->lhs = e->lhs;
					sub->rhs = e->rhs;

					sub->tree_type = e->tree_type;
					e->tree_type = decl_new();
					e->tree_type->type->primitive = type_int;

					e->op = op_divide;

					e->lhs = sub;
					e->rhs = expr_new_val(decl_size(cpy));
					fold_expr(e->rhs, stab);

					decl_free(cpy);

				}else{
					if(decl_ptr_depth(e->lhs->tree_type))
						/* lhs is the pointer, we're adding on rhs, hence multiply rhs by lhs's ptr size */
						e->rhs = expr_ptr_multiply(e->rhs, e->lhs->tree_type);
					else
						e->lhs = expr_ptr_multiply(e->lhs, e->rhs->tree_type);
				}

				{
					intval dummy;
					enum constyness d;
					const_fold(e, &dummy, &d);
				}
			}
		}


		if(e->op == op_shiftl || e->op == op_shiftr){
			if(decl_ptr_depth(e->rhs->tree_type))
				DIE_AT(&e->rhs->where, "invalid argument to shift");
			e->rhs->tree_type->type->primitive = type_char; /* force "shl ..., al" */
		}

		if(e->op == op_minus && IS_PTR(e->lhs) && IS_PTR(e->rhs)){
			/* ptr - ptr = int */
			/* FIXME: ptrdiff_t */
			e->tree_type->type->primitive = type_int;
		}
	}

#undef SPEL_IF_IDENT
#undef IS_PTR
}

void gen_expr_str_op(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("op: %s\n", op_to_str(e->op));
	gen_str_indent++;

	if(e->op == op_deref){
		idt_printf("deref size: %s ", decl_to_str(e->tree_type));
		fputc('\n', cc1_out);
	}

#define PRINT_IF(hs) if(e->hs) print_expr(e->hs)
	PRINT_IF(lhs);
	PRINT_IF(rhs);
#undef PRINT_IF

	gen_str_indent--;
}

static void op_shortcircuit(expr *e, symtable *tab)
{
	char *bail = out_label_code("shortcircuit_bail");
#ifdef SC_OPTIMISE
	char *bail_nonormalise = out_label_code("shortcircuit_bail_nonorm");
#endif

	gen_expr(e->lhs, tab);

	out_dup();
	out_jz(bail);

	gen_expr(e->rhs, tab);

	out_label(bail);
	free(bail);

	out_normalise();

#ifdef SC_OPTIMISE
	out_label(bail_nonormalise);
	free(bail_nonormalise);
#endif
}

void op_operate_struct(expr *e, symtable *tab)
{
	(void)tab;

	UCC_ASSERT(e->op == op_struct_ptr, "a.b should have been handled by now");

	gen_expr(e->lhs, tab);

	/* pointer to the struct is on the stack, get from the offset */
	out_comment("struct ptr");

	out_push_i(e->rhs->tree_type, e->rhs->tree_type->struct_offset);

	out_comment("offset of member %s", e->rhs->spel);

	if(decl_is_array(e->rhs->tree_type)){
		out_comment("array - got address");
	}else{
		out_op(op_deref, e->rhs->tree_type);
	}

	out_comment("val from struct");
}

void gen_expr_op(expr *e, symtable *tab)
{
	switch(e->op){
		case op_struct_dot:
		case op_struct_ptr:
			op_operate_struct(e, tab);
			break;

		case op_orsc:
		case op_andsc:
			op_shortcircuit(e, tab);
			break;

		case op_unknown:
			ICE("asm_operate: unknown operator got through");

		default:
			gen_expr(e->lhs, tab);

			if(e->rhs){
				gen_expr(e->rhs, tab);

				out_op(e->op, e->tree_type);
			}else{
				out_op_unary(e->op, e->tree_type);
			}
	}
}

void gen_expr_op_store(expr *store, symtable *stab)
{
	switch(store->op){
		case op_deref:
			/* a dereference */
			gen_expr(op_deref_expr(store), stab); /* skip over the *() bit */
			out_comment("pointer on stack");

			/* move `pop` into `pop` */
			out_store(store->tree_type);
			return;

		case op_struct_ptr:
			gen_expr(store->lhs, stab);

			out_push_i(NULL, store->rhs->tree_type->struct_offset);
			out_comment("offset of member %s", store->rhs->spel);

			out_store(store->tree_type);
			return;

		case op_struct_dot:
			ICE("a.b missed");
			break;

		default:
			break;
	}
	ICE("invalid store-op %s", op_to_str(store->op));
}

void mutate_expr_op(expr *e)
{
	e->f_store = gen_expr_op_store;
	e->f_fold = fold_expr_op;
	e->f_const_fold = fold_const_expr_op;
}

expr *expr_new_op(enum op_type op)
{
	expr *e = expr_new_wrapper(op);
	e->op = op;
	return e;
}

void gen_expr_style_op(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
