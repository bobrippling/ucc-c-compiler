#include <string.h>
#include <stdlib.h>
#include "ops.h"
#include "../struct.h"

const char *expr_str_op()
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
	struct_st *st;
	decl *d, **i;
	char *spel;

	fold_expr(e->lhs, stab);
	/* don't fold the rhs - just a member name */

	if(expr_kind(e->rhs, identifier))
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

void fold_typecheck_sign(where *where, expr *lhs, expr *rhs, enum op_type op)
{
	enum
	{
		SIGNED, UNSIGNED
	} rhs_signed, lhs_signed;
	type *type_l, *type_r;

	type_l = lhs->tree_type->type;
	type_r = rhs->tree_type->type;

	if(type_l->primitive == type_enum
			&& type_r->primitive == type_enum
			&& type_l->enu != type_r->enu){
		cc1_warn_at(where, 0, WARN_ENUM_CMP, "operation between enum %s and enum %s", type_l->spel, type_r->spel);
	}

	lhs_signed = type_l->spec & spec_unsigned ? UNSIGNED : SIGNED;
	rhs_signed = type_r->spec & spec_unsigned ? UNSIGNED : SIGNED;

	if(op_is_cmp(op) && rhs_signed != lhs_signed){
#define SIGN_CONVERT(hs) \
		if(hs->type == expr_val && hs->val.i.val >= 0){ \
			/*                                              \
				* assert(lhs_signed == UNSIGNED);                     \
				* vals default to signed, change to unsigned   \
				*/                                             \
			UCC_ASSERT(hs ## _signed == UNSIGNED,               \
					"signed-unsigned assumption failure");      \
																											\
			hs->tree_type->type->spec |= spec_unsigned; \
			goto noproblem;                                 \
		}

		SIGN_CONVERT(rhs)
		SIGN_CONVERT(lhs)

#define SPEL_IF_IDENT(hs)                              \
				hs->type == expr_identifier ? " ("     : "", \
				hs->type == expr_identifier ? hs->spel : "", \
				hs->type == expr_identifier ? ")"      : ""  \

		cc1_warn_at(where, 0, WARN_SIGN_COMPARE, "comparison between signed and unsigned%s%s%s%s%s%s",
				SPEL_IF_IDENT(lhs), SPEL_IF_IDENT(rhs));
	}

noproblem:
	return;
}

void fold_typecheck_primitive(expr **plhs, expr **prhs)
{
	expr *lhs, *rhs;

	lhs = *plhs;
	rhs = *prhs;

	if(lhs->tree_type->type->primitive != rhs->tree_type->type->primitive){
		/* insert a cast: rhs -> lhs */
		expr *cast = *prhs = expr_new();

		cast->type = expr_cast;

		cast->lhs = expr_new(); /* just the type */
		cast->lhs->tree_type = decl_copy(lhs->tree_type); /* cast to lhs */
		cast->rhs = rhs;

		GET_TREE_TYPE_TO(cast, lhs->tree_type);
	}
}

void fold_op_typecheck(expr *e, symtable *stab)
{
	fold_typecheck_sign(&e->where, e->lhs, e->rhs, e->op);
	fold_typecheck_primitive(&e->lhs, &e->rhs);
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

void expr_fold_op(expr *e, symtable *stab)
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
}

void expr_gen_str_op(expr *e)
{
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

	asm_temp(1, "xor rdx,rdx");
	asm_temp(1, "pop rbx");
	asm_temp(1, "pop rax");
	asm_temp(1, "idiv rbx");

	asm_temp(1, "push r%cx", e->op == op_divide ? 'a' : 'd');
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

void expr_gen_op(expr *e, symtable *tab)
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

			/* e.g. "movzx rax, byte [rax]" */
			asm_temp(1, "mov %sax, %s [rax]",
					asm_reg_name(e->tree_type),
					asm_type_str(e->tree_type));

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

void expr_gen_op_store(expr *e, symtable *stab)
{
	switch(e->op){
		case op_deref:
			/* a dereference */
			asm_push(store->tree_type, 'a');
			asm_comment("value to save");

			gen_expr(e->lhs, stab); /* skip over the *() bit */
			asm_comment("pointer on stack");

			/* move `pop` into `pop` */
			asm_pop(store->tree_type, 'a');
			asm_comment("address");


			asm_pop(store->tree_type, 'b');
			asm_comment("value");

			asm_mov(store->tree_type, asm_o HERE

			asm_output_new(
						asm_out_type_mov,
						asm_operand_new_deref(store->tree_type, asm_operand_new_reg(store->tree_type, 'a'), 0),
						asm_operand_new_reg(  store->lhs->tree_type, 'b')
					);
			return;

		case op_struct_ptr:
			gen_expr(e->lhs, stab);

			asm_pop('b');
			asm_comment("struct addr");

			asm_output_new(
					asm_out_type_add,
					asm_operand_new_reg(store->tree_type, 'b'),
					asm_operand_new_val(store->tree_type, store->rhs->tree_type->struct_offset)
				);
			asm_comment("offset of member %s", store->rhs->spel);

			asm_output_new(
						asm_out_type_mov,
						asm_operand_new_reg(store->tree_type, 'a'),
						asm_operand_new_deref(store->tree_type, "rsp", 0)
					);

			asm_output_new(
					asm_out_type_mov,
					asm_operand_new_reg('a'),
					/* TODO */
					1);

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
	e->f_store = expr_gen_op_store;
	e->op = op;
	return e;
}
