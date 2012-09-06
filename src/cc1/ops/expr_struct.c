#include "ops.h"
#include "expr_struct.h"
#include "../sue.h"
#include "../out/asm.h"

#define ASSERT_NOT_DOT() UCC_ASSERT(!e->expr_is_st_dot, "a.b should have been handled by now");

const char *str_expr_struct()
{
	return "struct";
}

void fold_expr_struct(expr *e, symtable *stab)
{
	/*
	 * lhs = any ptr-to-struct expr
	 * rhs = struct member ident
	 */
	const int ptr_expect = !e->expr_is_st_dot;
	struct_union_enum_st *sue;
	char *spel;

	fold_expr(e->lhs, stab);
	/* don't fold the rhs - just a member name */

	UCC_ASSERT(expr_kind(e->rhs, identifier),
			"struct member not identifier (%s)", e->rhs->f_str());
	spel = e->rhs->spel;

	/* we access a struct, of the right ptr depth */
	if(!decl_is_struct_or_union_possible_ptr(e->lhs->tree_type)
	||  decl_is_ptr(e->lhs->tree_type) != ptr_expect)
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

	if(sue_incomplete(sue)){
		DIE_AT(&e->lhs->where, "%s incomplete type (%s)",
				ptr_expect
					? "dereferencing pointer to"
					: "use of",
				type_to_str(e->lhs->tree_type->type));
	}

	/* found the struct, find the member */
	e->rhs->tree_type = struct_union_member_find(sue, spel, &e->where);

	/*
	 * if it's a.b, convert to (&a)->b for asm gen
	 * e = { lhs = "a", rhs = "b", type = dot }
	 * e = {
	 *   type = ptr,
	 *   lhs = { cast<void *>, expr = { expr = "a", type = addr } },
	 *   rhs = "b",
	 * }
	 */
	if(!ptr_expect){
		expr *cast, *addr;

		cast = expr_new_cast(decl_ptr_depth_inc(decl_new_void()), 1);
		cast->expr = addr = expr_new_addr();

		addr->lhs = e->lhs;

		e->lhs = cast;
		e->expr_is_st_dot = 0;

		fold_expr(e->lhs, stab);
	}

	e->tree_type = decl_copy(e->rhs->tree_type);
	/* pull qualifiers from the struct to the member */
	e->tree_type->type->qual |= e->lhs->tree_type->type->qual;
}

void gen_expr_struct_store(expr *e, symtable *stab)
{
	ASSERT_NOT_DOT();

	gen_expr(e->lhs, stab);

	out_push_i(NULL, struct_offset(e->rhs));
	out_op(op_plus);

	out_change_decl(decl_ptr_depth_inc(decl_copy(e->rhs->tree_type)));
}

void gen_expr_struct(expr *e, symtable *stab)
{
	(void)stab;

	ASSERT_NOT_DOT();

	gen_expr_struct_store(e, stab);

	if(decl_is_array(e->rhs->tree_type)){
		out_comment("array - got address");
	}else{
		out_deref();
	}

	out_comment("val from struct");
}

void gen_expr_str_struct(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("struct%s%s\n",
			e->expr_is_st_dot ? "." : "->",
			e->rhs->spel);

	gen_str_indent++;
	print_expr(e->lhs);
	gen_str_indent--;
}

void fold_const_expr_struct(expr *e, intval *val, enum constyness *success)
{
	/* if lhs is NULL, const fold to struct offset, (obv. if !dot, which is taken care of in fold) */
	ASSERT_NOT_DOT();

	const_fold(e->lhs, val, success);

	if(*success == CONST_WITH_VAL)
		val->val += struct_offset(e->rhs);
	else
		*success = CONST_WITHOUT_VAL;
}

void static_expr_struct_addr(expr *e)
{
	static_store(e->lhs);
	asm_declare_partial(" + %ld", struct_offset(e->rhs));
}

void mutate_expr_struct(expr *e)
{
	e->f_store = gen_expr_struct_store;
	e->f_static_addr = static_expr_struct_addr;
}

expr *expr_new_struct(expr *sub, int dot, expr *ident)
{
	expr *e = expr_new_wrapper(struct);
	e->expr_is_st_dot = dot;
	e->lhs = sub;
	e->rhs = ident;
	return e;
}

void gen_expr_style_struct(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
