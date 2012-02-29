#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "tree.h"
#include "cc1.h"
#include "gen_asm.h"
#include "asm.h"
#include "../util/util.h"
#include "asm_op.h"

#include "../data_structs.h"
#include "expr_op.h"

void fold_const_expr_op(expr *e)
{
	int l, r;
	l = const_fold(e->lhs);
	r = e->rhs ? const_fold(e->rhs) : 0;

#define VAL(x) x->type == expr_val

	if(!l && !r && VAL(e->lhs) && (e->rhs ? VAL(e->rhs) : 1)){
		int bad = 0;

		e->val.i.val = operate(e->lhs, e->rhs, e->op, &bad);

		if(!bad)
			e->type = expr_val;
		/*
			* TODO: else free e->[lr]hs
			* currently not a leak, just ignored
			*/

		return bad;
	}else{
		operate_optimise(e);
	}
#undef VAL
}

void fold_op_struct(expr *e, symtable *stab)
{
	/*
	 * lhs = any ptr-to-struct expr
	 * rhs = struct member ident
	 */
	const int ptr_depth_exp = e->op == op_struct_ptr ? 1 : 0;
	struct_st *st;
	decl *d, **i;
	char *spel;

	fold_expr(e->lhs, stab);
	/* don't fold the rhs - just a member name */

	if(e->rhs->type != expr_identifier)
		die_at(&e->rhs->where, "struct member must be an identifier");
	spel = e->rhs->spel;

	/* we access a struct, of the right ptr depth */
	if(e->lhs->tree_type->type->primitive != type_struct
			|| decl_ptr_depth(e->lhs->tree_type) != ptr_depth_exp)
		die_at(&e->lhs->where, "%s is not a %sstruct",
				decl_to_str(e->lhs->tree_type),
				ptr_depth_exp == 1 ? "pointer-to-" : "");

	st = e->lhs->tree_type->type->struc;

	if(!st)
		die_at(&e->lhs->where, "%s incomplete type",
				ptr_depth_exp == 1 /* this should always be true.. */
				? "dereferencing pointer to"
				: "use of");

	/* found the struct, find the member */
	d = NULL;
	for(i = st->members; i && *i; i++)
		if(!strcmp((*i)->spel, spel)){
			d = *i;
			break;
		}

	if(!d)
		die_at(&e->rhs->where, "struct %s has no member named \"%s\"", st->spel, spel);

	GET_TREE_TYPE_TO(e->rhs, d);

	/*
	 * if it's a.b, convert to (&a)->b for asm gen
	 * e = { lhs = "a", rhs = "b", type = dot }
	 * e = { lhs = { expr = "a", type = addr }, rhs = "b", type = ptr }
	 */
	if(ptr_depth_exp == 0){
		expr *new = expr_new();

		new->expr = e->lhs;
		e->lhs = new;
		new->type = expr_addr;

		e->type = expr_op;
		e->op   = op_struct_ptr;

		fold_expr(e->lhs, stab);
		GET_TREE_TYPE(e->lhs->tree_type);
	}else{
		GET_TREE_TYPE(d);
	}
}

void fold_op_typecheck(expr *e, symtable *stab)
{
	enum {
		SIGNED, UNSIGNED
	} rhs, lhs;
	type *type_l, *type_r;

	(void)stab;

	if(!e->rhs)
		return;

	type_l = e->lhs->tree_type->type;
	type_r = e->rhs->tree_type->type;

	if(type_l->primitive == type_enum
			&& type_r->primitive == type_enum
			&& type_l->enu != type_r->enu){
		cc1_warn_at(&e->where, 0, WARN_ENUM_CMP, "comparison between enum %s and enum %s", type_l->spel, type_r->spel);
	}


	lhs = type_l->spec & spec_unsigned ? UNSIGNED : SIGNED;
	rhs = type_r->spec & spec_unsigned ? UNSIGNED : SIGNED;

	if(op_is_cmp(e->op) && rhs != lhs){
#define SIGN_CONVERT(test_hs, assert_hs) \
		if(e->test_hs->type == expr_val && e->test_hs->val.i.val >= 0){ \
			/*                                              \
				* assert(lhs == UNSIGNED);                     \
				* vals default to signed, change to unsigned   \
				*/                                             \
			UCC_ASSERT(assert_hs == UNSIGNED,               \
					"signed-unsigned assumption failure");      \
																											\
			e->test_hs->tree_type->type->spec |= spec_unsigned; \
			goto noproblem;                                 \
		}

		SIGN_CONVERT(rhs, lhs)
		SIGN_CONVERT(lhs, rhs)

#define SPEL_IF_IDENT(hs)                              \
				hs->type == expr_identifier ? " ("     : "", \
				hs->type == expr_identifier ? hs->spel : "", \
				hs->type == expr_identifier ? ")"      : ""  \

		cc1_warn_at(&e->where, 0, WARN_SIGN_COMPARE, "comparison between signed and unsigned%s%s%s%s%s%s",
				SPEL_IF_IDENT(e->lhs), SPEL_IF_IDENT(e->rhs));
	}
noproblem:
	return;
}

void fold_deref(expr *e)
{
	/* check for *&x */
	if(e->lhs->type == expr_addr)
		warn_at(&e->lhs->where, "possible optimisation for *& expression");

	GET_TREE_TYPE(e->lhs->tree_type);

	e->tree_type = decl_ptr_depth_dec(e->tree_type);

	if(decl_ptr_depth(e->tree_type) == 0 && e->lhs->tree_type->type->primitive == type_void)
		die_at(&e->where, "can't dereference void pointer");
}

void fold_op(expr *e, symtable *stab)
{
	if(e->op == op_struct_ptr || e->op == op_struct_dot){
		fold_op_struct(e, stab);
		return;
	}

	fold_expr(e->lhs, stab);
	if(e->rhs)
		fold_expr(e->rhs, stab);

	fold_op_typecheck(e, stab);

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


#define IS_PTR(x) decl_ptr_depth(x->tree_type)
		if(e->rhs){
			if(e->op == op_minus && IS_PTR(e->lhs) && IS_PTR(e->rhs))
				e->tree_type->type->primitive = type_int;
			else if(IS_PTR(e->rhs))
				GET_TREE_TYPE(e->rhs->tree_type);
			else
				goto norm_tt;
		}else{
norm_tt:
			GET_TREE_TYPE(e->lhs->tree_type);
		}
	}

	if(e->rhs){
		/* need to do this check _after_ we get the correct tree type */
		if((e->op == op_plus || e->op == op_minus) &&
				decl_ptr_depth(e->tree_type) &&
				e->rhs){

			/* 2 + (void *)5 is 7, not 2 + 8*5 */
			if(e->tree_type->type->primitive != type_void){
				/* we're dealing with pointers, adjust the amount we add by */

				if(decl_ptr_depth(e->lhs->tree_type))
					/* lhs is the pointer, we're adding on rhs, hence multiply rhs by lhs's ptr size */
					e->rhs = expr_ptr_multiply(e->rhs, e->lhs->tree_type);
				else
					e->lhs = expr_ptr_multiply(e->lhs, e->rhs->tree_type);

				const_fold(e);
			}else{
				cc1_warn_at(&e->tree_type->type->where, 0, WARN_VOID_ARITH, "arithmetic with void pointer");
			}
		}

		/* check types */
		if(e->rhs){
			fold_decl_equal(e->lhs->tree_type, e->rhs->tree_type,
					&e->where, WARN_COMPARE_MISMATCH,
					"operation between mismatching types");
		}
	}
}


static void asm_idiv(expr *e, symtable *tab)
{
	/*
	 * idiv — Integer Division
	 * The idiv asm_temp divides the contents of the 64 bit integer EDX:EAX (constructed by viewing EDX as the most significant four bytes and EAX as the least significant four bytes) by the specified operand value. The quotient result of the division is stored into EAX, while the remainder is placed in EDX.
	 * Syntax
	 * idiv <reg32>
	 * idiv <mem>
	 *
	 * Examples
	 *
	 * idiv ebx — divide the contents of EDX:EAX by the contents of EBX. Place the quotient in EAX and the remainder in EDX.
	 * idiv DWORD PTR [var] — divide the contents of EDX:EAS by the 32-bit value stored at memory location var. Place the quotient in EAX and the remainder in EDX.
	 */

	walk_expr(e->lhs, tab);
	walk_expr(e->rhs, tab);
	/* pop top stack (rhs) into b, and next top into a */

	asm_temp(1, "xor rdx,rdx");
	asm_temp(1, "pop rbx");
	asm_temp(1, "pop rax");
	asm_temp(1, "idiv rbx");

	asm_temp(1, "push r%cx", e->op == op_divide ? 'a' : 'd');
}

static void asm_compare(expr *e, symtable *tab)
{
	const char *cmp = NULL;

	walk_expr(e->lhs, tab);
	walk_expr(e->rhs, tab);
	asm_temp(1, "pop rbx");
	asm_temp(1, "pop rax");
	asm_temp(1, "xor rcx,rcx"); /* must be before cmp */
	asm_temp(1, "cmp rax,rbx");

	/* check for unsigned, since signed isn't explicitly set */
#define SIGNED(s, u) e->tree_type->type->spec & spec_unsigned ? u : s

	switch(e->op){
		case op_eq: cmp = "e";  break;
		case op_ne: cmp = "ne"; break;

		case op_le: cmp = SIGNED("le", "be"); break;
		case op_lt: cmp = SIGNED("l",  "b");  break;
		case op_ge: cmp = SIGNED("ge", "ae"); break;
		case op_gt: cmp = SIGNED("g",  "a");  break;

		default:
			ICE("asm_compare: unhandled comparison");
	}

	asm_temp(1, "set%s cl", cmp);
	asm_temp(1, "push rcx");
}

static void asm_shortcircuit(expr *e, symtable *tab)
{
	char *baillabel = asm_label_code("shortcircuit_bail");
	walk_expr(e->lhs, tab);

	asm_temp(1, "mov rax,[rsp]");
	asm_temp(1, "test rax,rax");
	/* leave the result on the stack (if false) and bail */
	asm_temp(1, "j%sz %s", e->op == op_andsc ? "" : "n", baillabel);
	asm_temp(1, "pop rax");
	walk_expr(e->rhs, tab);

	asm_label(baillabel);
	free(baillabel);
}

void asm_operate_struct(expr *e, symtable *tab)
{
	(void)tab;

	UCC_ASSERT(e->op == op_struct_ptr, "a.b should have been handled by now");

	walk_expr(e->lhs, tab);

	/* pointer to the struct is on the stack, get from the offset */
	asm_temp(1, "pop rax ; struct ptr");
	asm_temp(1, "add rax, %d ; offset of member %s",
			e->rhs->tree_type->struct_offset,
			e->rhs->spel);
	asm_temp(1, "mov rax, [rax] ; val from struct");
	asm_temp(1, "push rax");
}

void asm_operate(expr *e, symtable *tab)
{
	const char *instruct = NULL;
	const char *rhs = "rcx";

	switch(e->op){
		/* normal mafs */
		case op_multiply: instruct = "imul"; break;
		case op_plus:     instruct = "add";  break;
		case op_xor:      instruct = "xor";  break;
		case op_or:       instruct = "or";   break;
		case op_and:      instruct = "and";  break;

		/* single register op */
		case op_minus: instruct = e->rhs ? "sub" : "neg"; break;
		case op_bnot:  instruct = "not";                  break;

#define SHIFT(side) \
		case op_shift ## side: instruct = "sh" # side; rhs = "cl"; break

		SHIFT(l);
		SHIFT(r);

		case op_not:
			/* compare with 0 */
			walk_expr(e->lhs, tab);
			asm_temp(1, "xor rbx,rbx");
			asm_temp(1, "pop rax");
			asm_temp(1, "test rax,rax");
			asm_temp(1, "setz bl");
			asm_temp(1, "push rbx");
			return;

		case op_deref:
			walk_expr(e->lhs, tab);
			asm_temp(1, "pop rax");

			if(asm_type_size(e->tree_type) == ASM_SIZE_WORD)
				asm_temp(1, "mov rax, [rax]");
			else
				asm_temp(1, "movzx rax, byte [rax]");

			asm_temp(1, "push rax");
			return;

		case op_struct_dot:
		case op_struct_ptr:
			asm_operate_struct(e, tab);
			return;

		/* comparison */
		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
			asm_compare(e, tab);
			return;

		/* shortcircuit */
		case op_orsc:
		case op_andsc:
			asm_shortcircuit(e, tab);
			return;

		case op_divide:
		case op_modulus:
			asm_idiv(e, tab);
			return;

		case op_unknown:
			ICE("asm_operate: unknown operator got through");
	}

	/* asm_temp(1, "%s rax", incr ? "inc" : "dec"); TODO: optimise */

	/* get here if op is *, +, - or ~ */
	walk_expr(e->lhs, tab);
	if(e->rhs){
		walk_expr(e->rhs, tab);
		asm_temp(1, "pop rcx");
		asm_temp(1, "pop rax");
		asm_temp(1, "%s rax, %s", instruct, rhs);
	}else{
		asm_temp(1, "pop rax");
		asm_temp(1, "%s rax", instruct);
	}

	asm_temp(1, "push rax");
}
