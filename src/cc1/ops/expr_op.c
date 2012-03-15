#include <string.h>
#include <stdlib.h>
#include "ops.h"
#include "../struct.h"

const char *str_expr_op()
{
	return "op";
}

/* FIXME: assumed to be signed long */
#define OP_EXEC(t, operator)        \
int exec_op_ ## t(op *op, int *bad) \
{                                   \
	(void)bad;                        \
	return op->lhs->val.iv.val operator op->rhs->val.iv.val;  \
}

OP_EXEC(multiply,   *)
OP_EXEC(modulus,    %)
OP_EXEC(eq,         ==)
OP_EXEC(ne,         !=)
OP_EXEC(le,         <=)
OP_EXEC(lt,         <)
OP_EXEC(ge,         >=)
OP_EXEC(gt,         >)
OP_EXEC(xor,        ^)
OP_EXEC(or,         |)
OP_EXEC(and,        &)
OP_EXEC(orsc,       ||)
OP_EXEC(andsc,      &&)
OP_EXEC(shiftl,     <<)
OP_EXEC(shiftr,     >>)
#undef OP_EXEC

#define OP_EXEC(t, operator)           \
int exec_op_ ## t(op *op, int *bad)    \
{                                      \
	(void)bad;                           \
	return operator op->lhs->val.iv.val; \
}

OP_EXEC(not,  !)
OP_EXEC(bnot, ~)

#undef OP_EXEC

int op_exec(op *op, int *bad)
{
	if(expr_kind(op->lhs, val) && expr_kind(op->rhs, val)){
		return op->f_exec(op, bad);
	}else{
		*bad = 1;
		return 0;
	}
}


void operate_optimise(expr *e)
{
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

int fold_const_expr_op(expr *e)
{
	int l, r;
	l = const_fold(e->lhs);
	r = e->rhs ? const_fold(e->rhs) : 0;

	if(!l && !r && expr_kind(e->lhs, val) && (e->rhs ? expr_kind(e->rhs, val) : 1)){
		int bad = 0;

		e->val.iv.val = e->op->f_exec(e->op, e->lhs, e->rhs, &bad);

		if(!bad)
			expr_mutate_wrapper(e, val);

		/*
		 * TODO: else free e->[lr]hs
		 * currently not a leak, just ignored
		 */

		return bad;
	}else{
		e->op->f_optimise(e->op, e->lhs, e->rhs);
	}
#undef VAL

	return 1;
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

	if(!expr_kind(e->rhs, identifier))
		die_at(&e->rhs->where, "struct member must be an identifier (got %s)", e->rhs->f_str());
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

	e->rhs->tree_type = d;

	/*
	 * if it's a.b, convert to (&a)->b for asm gen
	 * e = { lhs = "a", rhs = "b", type = dot }
	 * e = { lhs = { expr = "a", type = addr }, rhs = "b", type = ptr }
	 */
	if(ptr_depth_exp == 0){
		expr *new = expr_new_addr();

		new->expr = e->lhs;
		e->lhs = new;

		expr_mutate_wrapper(e, op);
		e->op   = op_struct_ptr;

		fold_expr(e->lhs, stab);
		e->tree_type = decl_copy(e->lhs->tree_type);
	}else{
		e->tree_type = decl_copy(d);
	}
}

void fold_deref(expr *e)
{
	/* check for *&x */
	if(expr_kind(e->lhs, addr))
		warn_at(&e->lhs->where, "possible optimisation for *& expression");

	e->tree_type = decl_ptr_depth_dec(decl_copy(e->lhs->tree_type));

	if(decl_ptr_depth(e->tree_type) == 0 && e->lhs->tree_type->type->primitive == type_void)
		die_at(&e->where, "can't dereference void pointer");
}

void fold_expr_op(expr *e, symtable *stab)
{
#define IS_PTR(x) decl_ptr_depth(x->tree_type)

#define SPEL_IF_IDENT(hs)                              \
					expr_kind(hs, identifier) ? " ("     : "", \
					expr_kind(hs, identifier) ? hs->spel : "", \
					expr_kind(hs, identifier) ? ")"      : ""  \

#define RHS e->rhs
#define LHS e->lhs

	enum {
		SIGNED, UNSIGNED
	} rhs_su, lhs_su;

	if(e->op == op_struct_ptr || e->op == op_struct_dot){
		fold_op_struct(e, stab);
		return;
	}

	fold_expr(e->lhs, stab);
	if(e->rhs){

		fold_expr(e->rhs, stab);

		fold_typecheck(e->lhs, e->rhs, stab, &e->where);

		lhs_su = e->lhs->tree_type->type->spec & spec_unsigned ? UNSIGNED : SIGNED;
		rhs_su = e->rhs->tree_type->type->spec & spec_unsigned ? UNSIGNED : SIGNED;

		if(op_is_cmp(e->op) && rhs_su != lhs_su){
			/*
			* assert(LHS == UNSIGNED);
			* vals default to signed, change to unsigned
			*/

			if(expr_kind(RHS, val) && RHS->val.iv.val >= 0){
				UCC_ASSERT(lhs_su == UNSIGNED, "signed-unsigned assumption failure");
				RHS->tree_type->type->spec |= spec_unsigned;
			}else if(expr_kind(LHS, val) && LHS->val.iv.val >= 0){
				UCC_ASSERT(rhs_su == UNSIGNED, "signed-unsigned assumption failure");
				LHS->tree_type->type->spec |= spec_unsigned;
			}else{
					cc1_warn_at(&e->where, 0, WARN_SIGN_COMPARE, "comparison between signed and unsigned%s%s%s%s%s%s",
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
			if(e->op == op_minus && IS_PTR(e->lhs) && IS_PTR(e->rhs)){
				e->tree_type = decl_new();
				e->tree_type->type->primitive = type_int;
			}else if(IS_PTR(e->rhs)){
				e->tree_type = decl_copy(e->rhs->tree_type);
			}else{
				goto norm_tt;
			}
		}else{
norm_tt:
			e->tree_type = decl_copy(e->lhs->tree_type);
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

#undef SPEL_IF_IDENT
#undef IS_PTR
}

void gen_expr_str_op(expr *e, symtable *stab)
{
	e->op->f_gen_str(e->op);
	(void)stab;
	idt_printf("op: %s\n", op_to_str(e->op));
	gen_str_indent++;

#define PRINT_IF(hs) if(e->hs) print_expr(e->hs)
	PRINT_IF(lhs);
	PRINT_IF(rhs);
#undef PRINT_IF

	gen_str_indent--;
}

static void asm_compare(expr *e, symtable *tab)
{
	const char *cmp = NULL;

	gen_expr(e->lhs, tab);
	gen_expr(e->rhs, tab);
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
	gen_expr(e->lhs, tab);

	asm_temp(1, "mov rax,[rsp]");
	asm_temp(1, "test rax,rax");
	/* leave the result on the stack (if false) and bail */
	asm_temp(1, "j%sz %s", e->op == op_andsc ? "" : "n", baillabel);
	asm_temp(1, "pop rax");
	gen_expr(e->rhs, tab);

	asm_label(baillabel);
	free(baillabel);
}

void asm_operate_struct(expr *e, symtable *tab)
{
	(void)tab;

	UCC_ASSERT(e->op == op_struct_ptr, "a.b should have been handled by now");

	gen_expr(e->lhs, tab);

	/* pointer to the struct is on the stack, get from the offset */
	asm_temp(1, "pop rax ; struct ptr");
	asm_temp(1, "add rax, %d ; offset of member %s",
			e->rhs->tree_type->struct_offset,
			e->rhs->spel);
	asm_temp(1, "mov rax, [rax] ; val from struct");
	asm_temp(1, "push rax");
}

void gen_expr_op(expr *e, symtable *tab)
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
			gen_expr(e->lhs, tab);
			asm_temp(1, "xor rbx,rbx");
			asm_temp(1, "pop rax");
			asm_temp(1, "test rax,rax");
			asm_temp(1, "setz bl");
			asm_temp(1, "push rbx");
			return;

		case op_deref:
			gen_expr(e->lhs, tab);
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
	gen_expr(e->lhs, tab);
	if(e->rhs){
		gen_expr(e->rhs, tab);
		asm_temp(1, "pop rcx");
		asm_temp(1, "pop rax");
		asm_temp(1, "%s rax, %s", instruct, rhs);
	}else{
		asm_temp(1, "pop rax");
		asm_temp(1, "%s rax", instruct);
	}

	asm_temp(1, "push rax");
}

void gen_expr_op_store(expr *e, symtable *stab)
{
	switch(e->op){
		case op_deref:
			/* a dereference */
			asm_temp(1, "push rax ; save val");

			gen_expr(e->lhs, stab); /* skip over the *() bit */
			/* pointer on stack */

			/* move `pop` into `pop` */
			asm_temp(1, "pop rax ; ptr");
			asm_temp(1, "pop rbx ; val");
			asm_temp(1, "mov [rax], rbx");
			return;

		case op_struct_ptr:
			gen_expr(e->lhs, stab);

			asm_temp(1, "pop rbx ; struct addr");
			asm_temp(1, "add rbx, %d ; offset of member %s",
					e->rhs->tree_type->struct_offset,
					e->rhs->spel);
			asm_temp(1, "mov rax, [rsp] ; saved val");
			asm_temp(1, "mov [rbx], rax");
			return;

		case op_struct_dot:
			ICE("TODO: a.b");
			break;

		default:
			break;
	}
	ICE("invalid store-op %s", op_to_str(e->op));
}

expr *expr_new_op(enum op_type op)
{
	expr *e = expr_new_wrapper(op);
	e->f_store = gen_expr_op_store;
	e->op = op;
	return e;
}
