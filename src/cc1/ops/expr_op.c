#include <string.h>
#include <stdlib.h>
#include "ops.h"
#include "../sue.h"

/* no tt, since we want to clean the whole register */
#define ASM_XOR(reg)                              \
	asm_output_new(asm_out_type_xor,                \
			asm_operand_new_reg(NULL, ASM_REG_ ## reg), \
			asm_operand_new_reg(NULL, ASM_REG_ ## reg))

const char *str_expr_op()
{
	return "op";
}

int operate(expr *lhs, expr *rhs, enum op_type op, int *bad)
{
#define OP(a, b) case a: return lhs->val.iv.val b rhs->val.iv.val
	if(op != op_deref && !expr_kind(lhs, val)){
		*bad = 1;
		return 0;
	}

	switch(op){
		OP(op_multiply,   *);
		OP(op_modulus,    %);
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

		case op_divide:
			if(rhs->val.iv.val)
				return lhs->val.iv.val / rhs->val.iv.val;
			warn_at(&rhs->where, "division by zero");
			*bad = 1;
			return 0;

		case op_plus:
			if(rhs)
				return lhs->val.iv.val + rhs->val.iv.val;
			return lhs->val.iv.val;

		case op_minus:
			if(rhs)
				return lhs->val.iv.val - rhs->val.iv.val;
			return -lhs->val.iv.val;

		case op_not:  return !lhs->val.iv.val;
		case op_bnot: return ~lhs->val.iv.val;

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
				if(lhs->array_store->type == array_str)
					return *lhs->val.s;
				/*return lhs->val.exprs[0]->val.i;*/
			}
			*bad = 1;
			return 0;

		case op_struct_ptr:
		case op_struct_dot:
			*bad = 1;
			return 0;

		case op_unknown:
			break;
	}

	ICE("unhandled asm operate type");
	return 0;
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

int fold_const_expr_op(expr *e)
{
	int l, r;
	l = const_fold(e->lhs);
	r = e->rhs ? const_fold(e->rhs) : 0;

	if(!l && !r && expr_kind(e->lhs, val) && (e->rhs ? expr_kind(e->rhs, val) : 1)){
		int bad = 0;

		e->val.iv.val = operate(e->lhs, e->rhs, e->op, &bad);

		if(!bad)
			expr_mutate_wrapper(e, val);

		/*
		 * TODO: else free e->[lr]hs
		 * currently not a leak, just ignored
		 */

		return bad;
	}else{
		operate_optimise(e);
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
	struct_union_enum_st *st;
	char *spel;

	fold_expr(e->lhs, stab);
	/* don't fold the rhs - just a member name */

	if(!expr_kind(e->rhs, identifier))
		die_at(&e->rhs->where, "struct member must be an identifier (got %s)", e->rhs->f_str());
	spel = e->rhs->spel;

	/* we access a struct, of the right ptr depth */
	if(!decl_is_struct_or_union(e->lhs->tree_type)
	|| decl_ptr_depth(e->lhs->tree_type) != ptr_depth_exp)
	{
		const int ident = expr_kind(e->lhs, identifier);

		die_at(&e->lhs->where, "%s%s%s is not a %sstruct or union (member %s)",
				decl_to_str(e->lhs->tree_type),
				ident ? " " : "",
				ident ? e->lhs->spel : "",
				ptr_depth_exp == 1 ? "pointer-to-" : "",
				spel);
	}

	st = e->lhs->tree_type->type->sue;

	if(sue_incomplete(st))
		die_at(&e->lhs->where, "%s incomplete type (%s)",
				ptr_depth_exp == 1
				? "dereferencing pointer to"
				: "use of",
				type_to_str(e->lhs->tree_type->type));

	/* found the struct, find the member */
	e->rhs->tree_type = struct_union_member_find(st, spel, &e->where);

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

		new->expr = e->lhs;
		e->lhs = new;

		expr_mutate_wrapper(e, op);
		e->op   = op_struct_ptr;

		fold_expr(e->lhs, stab);
	}

	e->tree_type = decl_copy(e->rhs->tree_type);
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

	if(e->op == op_struct_ptr || e->op == op_struct_dot){
		fold_op_struct(e, stab);
		return;
	}

	fold_expr(e->lhs, stab);
	fold_disallow_st_un(e->lhs, "op-lhs");

	if(e->rhs){
		fold_expr(e->rhs, stab);
		fold_disallow_st_un(e->rhs, "op-rhs");

		/* check here? */
		fold_typecheck(e->lhs, &e->rhs, stab, &e->where);
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
			e->tree_type = decl_copy(e->lhs->tree_type);
		}
	}

	if(e->rhs){

		/* need to do this check _after_ we get the correct tree type */
		if((e->op == op_plus || e->op == op_minus)
		&& decl_ptr_depth(e->tree_type)
		&& e->rhs
		&& !e->op_no_ptr_mul)
		{
			/* 2 + (void *)5 is 7, not 2 + 8*5 */
			if(e->tree_type->type->primitive != type_void){
				/* we're dealing with pointers, adjust the amount we add by */

				if(e->op == op_minus){
					/* need to apply the divide to the current 'e' */
					expr *sub = expr_new_op(e->op);

					memcpy(&sub->where, &e->where, sizeof sub->where);

					sub->lhs = e->lhs;
					sub->rhs = e->rhs;
					sub->tree_type = e->tree_type;

					e->op = op_divide;

					e->lhs = sub;
					e->rhs = expr_new_val(decl_size(e->tree_type));
					fold_expr(e->rhs, stab);

					e->tree_type = decl_new();
					e->tree_type->type->primitive = type_int;

				}else{
					if(decl_ptr_depth(e->lhs->tree_type))
						/* lhs is the pointer, we're adding on rhs, hence multiply rhs by lhs's ptr size */
						e->rhs = expr_ptr_multiply(e->rhs, e->lhs->tree_type);
					else
						e->lhs = expr_ptr_multiply(e->lhs, e->rhs->tree_type);
				}

				const_fold(e);
			}else{
				cc1_warn_at(&e->tree_type->type->where, 0, WARN_VOID_ARITH, "arithmetic with void pointer");
			}
		}


		if(e->op == op_shiftl || e->op == op_shiftr){
			if(decl_ptr_depth(e->rhs->tree_type))
				die_at(&e->rhs->where, "invalid argument to shift");
			e->rhs->tree_type->type->primitive = type_char; /* force "shl ..., al" */
		}

		if(e->op == op_minus && IS_PTR(e->lhs) && IS_PTR(e->rhs)){
			/* ptr - ptr = int */
			/* FIXME: ptrdiff_t */
			e->tree_type->type->primitive = type_int;
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

	gen_expr(e->lhs, tab);
	gen_expr(e->rhs, tab);
	/* pop top stack (rhs) into b, and next top into a */

	/*
	xor rdx,rdx
	pop rbx
	pop rax
	idiv rbx
	push r%cx, e->op == op_divide ? 'a' : 'd'
	*/

	ASM_XOR(D);
	asm_pop(e->lhs->tree_type, ASM_REG_B);
	asm_pop(e->rhs->tree_type, ASM_REG_A);
	asm_output_new(asm_out_type_idiv, asm_operand_new_reg(e->tree_type, ASM_REG_A), NULL);
	asm_push(e->op == op_divide ? ASM_REG_A : ASM_REG_D);
}

static void asm_compare(expr *e, symtable *tab)
{
	const char *cmp = NULL;

	gen_expr(e->lhs, tab);
	gen_expr(e->rhs, tab);

	/*
	asm_temp(1, "pop rbx");
	asm_temp(1, "pop rax");
	asm_temp(1, "xor rcx,rcx"); * must be before cmp *
	asm_temp(1, "cmp rax,rbx");
	*/

	asm_pop(e->tree_type, ASM_REG_B); /* assume they've been converted by now */
	asm_pop(e->tree_type, ASM_REG_A);
	ASM_XOR(C);
	asm_output_new(asm_out_type_cmp,
			asm_operand_new_reg(e->tree_type, ASM_REG_A),
			asm_operand_new_reg(e->tree_type, ASM_REG_B));

	/* check for unsigned, since signed isn't explicitly set */
#define SIGNED(s, u) e->tree_type->type->is_signed ? s : u

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

	UCC_ASSERT(cmp, "no compare for op %s", op_to_str(e->op));

	asm_set(cmp, ASM_REG_C);
	asm_push(ASM_REG_C);
}

static void asm_shortcircuit(expr *e, symtable *tab)
{
	char *baillabel = asm_label_code("shortcircuit_bail");
	gen_expr(e->lhs, tab);

	asm_output_new(
			asm_out_type_mov,
			asm_operand_new_reg(  e->lhs->tree_type, ASM_REG_A),
			asm_operand_new_deref(e->lhs->tree_type, asm_operand_new_reg(NULL, ASM_REG_SP), 0)
		);

	ASM_TEST(e->lhs->tree_type, ASM_REG_A);

	/* leave the result on the stack (if false) and bail */
	asm_jmp_if_zero(e->op != op_andsc, baillabel);

	asm_pop(NULL, ASM_REG_A); /* ignore result */
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
	asm_pop(NULL, ASM_REG_A);
	asm_comment("struct ptr");

	asm_output_new(
			asm_out_type_add,
			asm_operand_new_reg(NULL, ASM_REG_A), /* pointer to struct */
			asm_operand_new_val(e->rhs->tree_type->struct_offset)
		);

	asm_comment("offset of member %s", e->rhs->spel);

	asm_output_new(
			asm_out_type_mov,
			asm_operand_new_reg(  e->rhs->tree_type, ASM_REG_A),
			asm_operand_new_deref(e->rhs->tree_type, asm_operand_new_reg(NULL, ASM_REG_A), 0)
		);

	asm_comment("val from struct");

	asm_push(ASM_REG_A);
}

void gen_expr_op(expr *e, symtable *tab)
{
	asm_out_func *instruct = NULL;

	switch(e->op){
		/* normal mafs */
		case op_multiply: instruct = asm_out_type_imul; break;
		case op_plus:     instruct = asm_out_type_add;  break;
		case op_xor:      instruct = asm_out_type_xor;  break;
		case op_or:       instruct = asm_out_type_or;   break;
		case op_and:      instruct = asm_out_type_and;  break;

		/* single register op */
		case op_minus: instruct = e->rhs ? asm_out_type_sub : asm_out_type_neg; break;
		case op_bnot:  instruct = asm_out_type_not;                  break;

#define SHIFT(side) \
		case op_shift ## side: instruct = asm_out_type_sh ## side; break

		/*rnote - sh[lr] needs rhs to be cl */
		SHIFT(l);
		SHIFT(r);

		case op_not:
			/* compare with 0 */
			gen_expr(e->lhs, tab);
			/*ASM_XOR(B); don't know why this was here */
			asm_pop(e->lhs->tree_type, ASM_REG_B);
			ASM_TEST(NULL, ASM_REG_A);
			asm_set("z", ASM_REG_B); /* setz bl */
			asm_push(ASM_REG_B);
			return;

		case op_deref:
			gen_expr(e->lhs, tab);
			asm_pop(NULL, ASM_REG_A);

			/* e.g. "movzx rax, byte [rax]" */
			asm_output_new(
					asm_out_type_mov,
					asm_operand_new_reg(  e->tree_type, ASM_REG_A),
					asm_operand_new_deref(NULL /* pointer */, asm_operand_new_reg(NULL, ASM_REG_A), 0));
			/* "mov %sax, %s [rax]",
					asm_reg_name(e->tree_type),
					asm_type_str(e->tree_type) */

			asm_push(ASM_REG_A);
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

		asm_pop(e->lhs->tree_type, ASM_REG_A);
		asm_pop(e->rhs->tree_type, ASM_REG_C);

		asm_output_new(
				instruct,
				asm_operand_new_reg(e->lhs->tree_type, ASM_REG_A),
				asm_operand_new_reg(e->rhs->tree_type, ASM_REG_C)
			);
		/*asm_temp(1, "%s rax, %s", instruct, rhs);*/
	}else{
		asm_pop(e->tree_type, ASM_REG_A);
		asm_output_new(
				instruct,
				asm_operand_new_reg(e->tree_type, ASM_REG_A),
				NULL);
	}

	asm_push(ASM_REG_A);
}

void gen_expr_op_store(expr *store, symtable *stab)
{
	switch(store->op){
		case op_deref:
			/* a dereference */
			asm_push(ASM_REG_A);
			asm_comment("value to save");

			gen_expr(store->lhs, stab); /* skip over the *() bit */
			asm_comment("pointer on stack");

			/* move `pop` into `pop` */
			asm_pop(NULL, ASM_REG_A);
			asm_comment("address");


			asm_pop(store->tree_type, ASM_REG_B);
			asm_comment("value");

			asm_output_new(
						asm_out_type_mov,
						asm_operand_new_deref(store->tree_type, asm_operand_new_reg(NULL, ASM_REG_A), 0),
						asm_operand_new_reg(  store->lhs->tree_type, ASM_REG_B)
					);
			return;

		case op_struct_ptr:
			gen_expr(store->lhs, stab);

			asm_pop(NULL, ASM_REG_B);
			asm_comment("struct addr");

			asm_output_new(
					asm_out_type_add,
					asm_operand_new_reg(store->tree_type, ASM_REG_B),
					asm_operand_new_val(store->rhs->tree_type->struct_offset)
				);
			asm_comment("offset of member %s", store->rhs->spel);

			asm_output_new(
						asm_out_type_mov,
						asm_operand_new_reg(  store->tree_type, ASM_REG_A),
						asm_operand_new_deref(store->tree_type, asm_operand_new_reg(NULL, ASM_REG_SP), 0)
					);
			/*asm_temp(1, "mov rax, [rsp] ; saved val");*/

			asm_output_new(
					asm_out_type_mov,
					asm_operand_new_deref(store->tree_type, asm_operand_new_reg(store->tree_type, ASM_REG_B), 0),
					asm_operand_new_reg(  store->tree_type, ASM_REG_A)
				);

			/*asm_temp(1, "mov [rbx], rax");*/
			return;

		case op_struct_dot:
			ICE("TODO: a.b");
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
