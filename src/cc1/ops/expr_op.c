#include <string.h>
#include <stdlib.h>
#include "ops.h"
#include "../sue.h"
#include "expr_op.h"
#include "../out/lbl.h"

const char *str_expr_op()
{
	return "op";
}

static void operate(
		intval *lval, intval *rval,
		enum op_type op,
		intval *piv, enum constyness *pconst_type,
		where *where)
{
#define OP(a, b) case a: piv->val = lval->val b rval->val; return
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
			long l = lval->val, r = rval->val;

			if(r){
				piv->val = op == op_divide ? l / r : l % r;
				return;
			}
			warn_at(where, 1, "division by zero");
			*pconst_type = CONST_NO;
			return;
		}

		case op_plus:
			piv->val = lval->val + (rval ? rval->val : 0);
			return;

		case op_minus:
			piv->val = rval ? lval->val - rval->val : -lval->val;
			return;

		case op_not:  piv->val = !lval->val; return;
		case op_bnot: piv->val = ~lval->val; return;

		case op_struct_ptr:
		case op_struct_dot:
			*pconst_type = CONST_NO;
			return;

		case op_unknown:
			break;
	}

	ICE("unhandled type");
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
		operate(&lhs, e->rhs ? &rhs : NULL, e->op, piv, pconst_type, &e->where);
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
	const int ptr_expect = e->op == op_struct_ptr;
	struct_union_enum_st *sue;
	char *spel;

	fold_expr(e->lhs, stab);
	/* don't fold the rhs - just a member name */

	if(!expr_kind(e->rhs, identifier))
		DIE_AT(&e->rhs->where, "struct member must be an identifier (got %s)", e->rhs->f_str());
	spel = e->rhs->spel;

	/* we access a struct, of the right ptr depth */
	if(!decl_is_struct_or_union_possible_ptr(e->lhs->tree_type)
	|| decl_is_ptr(e->lhs->tree_type) != ptr_expect)
	{
		const int ident = expr_kind(e->lhs, identifier);

		DIE_AT(&e->lhs->where, "%s%s%s is not a %sstruct or union (member %s)",
				decl_to_str(e->lhs->tree_type),
				ident ? " " : "",
				ident ? e->lhs->spel : "",
				ptr_expect ? "pointer to " : "",
				spel);
	}

	sue = e->lhs->tree_type->type->sue;

	if(sue_incomplete(sue))
		DIE_AT(&e->lhs->where, "%s incomplete type (%s)",
				ptr_expect
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
	if(!ptr_expect){
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

#define SPEL_IF_IDENT(hs)                            \
					expr_kind(hs, identifier) ? " ("     : "", \
					expr_kind(hs, identifier) ? hs->spel : "", \
					expr_kind(hs, identifier) ? ")"      : ""  \

static void expr_promote_int(expr **pe, enum type_primitive to, symtable *stab)
{
	expr *const e = *pe;
	expr *cast;

	if(type_primitive_size(e->tree_type->type->primitive) >= type_primitive_size(to))
		return;

  cast = expr_new_cast(decl_new_type(to));

	cast->expr = e;

	fold_expr_cast_descend(cast, stab, 0);

	*pe = cast;
}

decl *op_promote_types(
		enum op_type op,
		expr **plhs, expr **prhs,
		where *w, symtable *stab)
{
	decl *resolved = NULL;
	expr *lhs = *plhs, *rhs = *prhs;
	decl *const dlhs = lhs->tree_type, *const drhs = rhs->tree_type;

#if 0
	If either operand is a pointer:
		relation: both must be pointers

	if both operands are pointers:
		must be '-'

	exactly one pointer:
		must be + or -
#endif

	if(decl_is_void(dlhs) || decl_is_void(drhs))
		DIE_AT(w, "use of void expression");

	{
		const int l_ptr = decl_is_ptr(dlhs);
		const int r_ptr = decl_is_ptr(drhs);

		if(l_ptr && r_ptr){
			if(op != op_minus && !op_is_relational(op))
				DIE_AT(w, "operation between two pointers must be relational or subtraction");

			resolved = decl_new_type(type_int);

			goto fin;

		}else if(l_ptr ^ r_ptr){
			/* + or - */
			if(op != op_plus && op != op_minus)
				DIE_AT(w, "operation between pointer and integer must be + or -");

			resolved = decl_copy(l_ptr ? dlhs : drhs);

			/* FIXME: promote to unsigned */
			expr_promote_int(
					l_ptr ? &rhs : &lhs,
					op == op_plus ? type_intptr : type_ptrdiff,
					stab);

			goto fin;

		}else{
			UCC_ASSERT(!(l_ptr || r_ptr), "logic error");

		}
	}

#if 0
	If both operands have the same type, then no further conversion is needed.

	Otherwise, if both operands have signed integer types or both have unsigned
	integer types, the operand with the type of lesser integer conversion rank
	is converted to the type of the operand with greater rank.

	Otherwise, if the operand that has unsigned integer type has rank greater
	or equal to the rank of the type of the other operand, then the operand
	with signed integer type is converted to the type of the operand with
	unsigned integer type.

	Otherwise, if the type of the operand with signed integer type can
	represent all of the values of the type of the operand with unsigned
	integer type, then the operand with unsigned integer type is converted to
	the type of the operand with signed integer type.

	Otherwise, both operands are converted to the unsigned integer type
	corresponding to the type of the operand with signed integer type.
#endif

	{
		decl *dlarger = dlhs;

		if(dlhs->type->primitive != drhs->type->primitive){
			const int l_larger = decl_size(dlhs) > decl_size(drhs);

			/* TODO: needed? */
			fold_decl_equal(dlhs, drhs,
					w, WARN_COMPARE_MISMATCH,
					"operation between mismatching types%s%s%s%s%s%s",
					SPEL_IF_IDENT(lhs), SPEL_IF_IDENT(rhs));

			expr_promote_int(
					 l_larger ? &rhs : &lhs,
					(l_larger ? dlhs : drhs)->type->primitive,
					stab);

			dlarger = l_larger ? dlhs : drhs;
		}

		/* if we have a _comparison_ (e.g. between enums), convert to int */
		resolved = op_is_relational(op)
			? decl_new_type(type_int)
			: decl_copy(dlarger);
	}

fin:
	UCC_ASSERT(resolved, "no decl from type promotion");

	*plhs = lhs;
	*prhs = rhs;

	return resolved;
}
#undef SPEL_IF_IDENT
#undef IS_PTR

void fold_expr_op(expr *e, symtable *stab)
{
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

		e->tree_type = op_promote_types(e->op, &e->lhs, &e->rhs, &e->where, stab);
	}else{
		e->tree_type = decl_copy(e->lhs->tree_type);
	}
}

void gen_expr_str_op(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("op: %s\n", op_to_str(e->op));
	gen_str_indent++;

#define PRINT_IF(hs) if(e->hs) print_expr(e->hs)
	PRINT_IF(lhs);
	PRINT_IF(rhs);
#undef PRINT_IF

	gen_str_indent--;
}

static void op_shortcircuit(expr *e, symtable *tab)
{
	char *bail = out_label_code("shortcircuit_bail");

	gen_expr(e->lhs, tab);

	out_dup();
	(e->op == op_andsc ? out_jfalse : out_jtrue)(bail);
	out_pop();

	gen_expr(e->rhs, tab);

	out_label(bail);
	free(bail);

	out_normalise();
}

static void op_struct(expr *e, symtable *tab)
{
	(void)tab;

	UCC_ASSERT(e->op == op_struct_ptr, "a.b should have been handled by now");

	gen_expr(e->lhs, tab);

	/* pointer to the struct is on the stack, get from the offset */
	out_comment("struct ptr");

	out_push_i(NULL, e->rhs->tree_type->struct_offset);
	out_op(op_plus);

	out_change_decl(decl_ptr_depth_inc(decl_copy(e->rhs->tree_type)));
	out_comment("offset of member %s", e->rhs->spel);

	if(decl_is_array(e->rhs->tree_type)){
		out_comment("array - got address");
	}else{
		out_deref();
	}

	out_comment("val from struct");
}

void gen_expr_op(expr *e, symtable *tab)
{
	switch(e->op){
		case op_struct_dot:
		case op_struct_ptr:
			op_struct(e, tab);
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

				out_op(e->op);
			}else{
				out_op_unary(e->op);
			}
	}
}

void gen_expr_op_store(expr *store, symtable *stab)
{
	switch(store->op){
		case op_struct_ptr:
			gen_expr(store->lhs, stab);

			out_push_i(NULL, store->rhs->tree_type->struct_offset);
			out_op(op_plus);
			out_comment("offset of member %s", store->rhs->spel);
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
