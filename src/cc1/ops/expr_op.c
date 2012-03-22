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
	if(e->op->f_optimise)
		e->op->f_optimise(e->op);
}

int fold_const_expr_op(expr *e)
{
	int l, r;
	l = const_fold(e->op->lhs);
	r = e->op->rhs ? const_fold(e->op->rhs) : 0;

	if(!l && !r && expr_kind(e->op->lhs, val) && (e->op->rhs ? expr_kind(e->op->rhs, val) : 1)){
		int bad = 0;

		e->val.iv.val = e->op->f_exec(e->op, &bad);

		if(!bad)
			expr_mutate_wrapper(e, val);

		/*
		 * TODO: else free e->[lr]hs
		 * currently not a leak, just ignored
		 */

		return bad;
	}else{
		if(e->op->f_optimise)
			e->op->f_optimise(e->op);
	}

	return 1;
}

void fold_expr_op(expr *e, symtable *stab)
{
#define IS_PTR(x) decl_ptr_depth((x)->tree_type)

#define SPEL_IF_IDENT(hs)                              \
					expr_kind(hs, identifier) ? " ("     : "", \
					expr_kind(hs, identifier) ? hs->spel : "", \
					expr_kind(hs, identifier) ? ")"      : ""  \

#define RHS e->op->rhs
#define LHS e->op->lhs

	fold_expr(e->op->lhs, stab);
	if(e->op->rhs){
		fold_expr(e->op->rhs, stab);

		fold_typecheck(e->op->lhs, e->op->rhs, stab, &e->where);
	}

	e->op->f_fold(e->op, stab);


	/*
	 * look either side - if either is a pointer, take that as the tree_type
	 *
	 * operation between two values of any type
	 *
	 * TODO: checks for pointer + pointer (invalid), etc etc
	 */
	if(e->op->rhs){
		if(op_kind(e->op, minus) && IS_PTR(e->op->lhs) && IS_PTR(e->op->rhs)){
			e->tree_type = decl_new();
			e->tree_type->type->primitive = type_int;
		}else if(IS_PTR(e->op->rhs)){
			e->tree_type = decl_copy(e->op->rhs->tree_type);
		}else{
			goto norm_tt;
		}
	}else{
norm_tt:
		e->tree_type = decl_copy(e->op->lhs->tree_type);
	}

	if(e->op->rhs){
		/* need to do this check _after_ we get the correct tree type */
		if((op_kind(e->op, plus) || op_kind(e->op, minus)) && decl_ptr_depth(e->tree_type)){

			/* 2 + (void *)5 is 7, not 2 + 8*5 */
			if(e->tree_type->type->primitive != type_void){
				/* we're dealing with pointers, adjust the amount we add by */

				if(decl_ptr_depth(e->op->lhs->tree_type))
					/* lhs is the pointer, we're adding on rhs, hence multiply rhs by lhs's ptr size */
					e->op->rhs = expr_ptr_multiply(e->op->rhs, e->op->lhs->tree_type);
				else
					e->op->lhs = expr_ptr_multiply(e->op->lhs, e->op->rhs->tree_type);

				const_fold(e);
			}else{
				cc1_warn_at(&e->tree_type->type->where, 0, WARN_VOID_ARITH, "arithmetic with void pointer");
			}
		}

		/* check types */
		fold_decl_equal(e->op->lhs->tree_type, e->op->rhs->tree_type,
				&e->where, WARN_COMPARE_MISMATCH,
				"operation between mismatching types");
	}

#undef SPEL_IF_IDENT
#undef IS_PTR
}

void gen_expr_str_op(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("op: %s\n", e->op->f_str());
	gen_str_indent++;
  e->op->f_gen_str(e->op);
	gen_str_indent--;
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
