#include "ops.h"
#include "expr_struct.h"
#include "../sue.h"
#include "../out/asm.h"

#define ASSERT_NOT_DOT() UCC_ASSERT(!e->expr_is_st_dot, "a.b should have been handled by now")

#define struct_offset(rhs) (rhs)->val.decl->struct_offset

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

	FOLD_EXPR(e->lhs, stab);
	/* don't fold the rhs - just a member name */

	UCC_ASSERT(expr_kind(e->rhs, identifier),
			"struct/union member not identifier (%s)", e->rhs->f_str());
	spel = e->rhs->spel;

	/* we access a struct, of the right ptr depth */
	{
		type_ref *r = e->lhs->tree_type;

		if(ptr_expect){
			type_ref *rtest = type_ref_is(r, type_ref_ptr);

			if(!rtest && !(rtest = type_ref_is(r, type_ref_array)))
				goto err;

			r = rtest->ref; /* safe - rtest is an array */
		}

		if(!(sue = type_ref_is_s_or_u(r))){
			int ident;
err:
			ident = expr_kind(e->lhs, identifier);

			DIE_AT(&e->lhs->where, "'%s%s%s' is not a %sstruct or union (member %s)",
					type_ref_to_str(e->lhs->tree_type),
					ident ? " " : "",
					ident ? e->lhs->spel : "",
					ptr_expect ? "pointer to " : "",
					spel);
		}
	}

	if(sue_incomplete(sue)){
		DIE_AT(&e->lhs->where, "%s incomplete type (%s)",
				ptr_expect
					? "dereferencing pointer to"
					: "accessing member of",
				type_ref_to_str(e->lhs->tree_type));
	}

	/* found the struct, find the member */
	e->rhs->tree_type = (
		e->rhs->val.decl = struct_union_member_find(sue, spel, &e->where)
	)->ref;

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

		cast = expr_new_cast(type_ref_new_VOID_PTR(), 1);
		cast->expr = addr = expr_new_addr();

		addr->lhs = e->lhs;

		e->lhs = cast;
		e->expr_is_st_dot = 0;

		FOLD_EXPR(e->lhs, stab);
	}

	/* pull qualifiers from the struct to the member */
	{
		enum type_qualifier addon = type_ref_qual(e->lhs->tree_type);

		e->tree_type = type_ref_new_cast_add(e->rhs->tree_type, addon);
	}
}

void gen_expr_struct_lea(expr *e, symtable *stab)
{
	ASSERT_NOT_DOT();

	gen_expr(e->lhs, stab);

	out_change_type(type_ref_new_VOID_PTR()); /* cast for void* arithmetic */
	out_push_i(type_ref_new_INTPTR_T(), struct_offset(e->rhs)); /* integral offset */
	out_op(op_plus);

	out_change_type(type_ref_ptr_depth_inc(e->rhs->tree_type));
}

void gen_expr_struct(expr *e, symtable *stab)
{
	(void)stab;

	ASSERT_NOT_DOT();

	gen_expr_struct_lea(e, stab);

	out_deref();

	out_comment("val from struct/union");
}

void gen_expr_str_struct(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("struct/union%s%s\n",
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
	ASSERT_NOT_DOT();

	static_store(e->lhs);
	asm_declare_partial(" + %ld", struct_offset(e->rhs));
}

void mutate_expr_struct(expr *e)
{
	e->f_lea = gen_expr_struct_lea;
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
