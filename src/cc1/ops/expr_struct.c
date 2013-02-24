#include "ops.h"
#include "expr_struct.h"
#include "../sue.h"
#include "../out/asm.h"

#define ASSERT_NOT_DOT() UCC_ASSERT(!e->expr_is_st_dot, "a.b should have been handled by now")

#define struct_offset(e) ((e)->bits.struct_mem.d->struct_offset + (e)->bits.struct_mem.extra_off)

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
	spel = e->rhs->bits.ident.spel;

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
err:
			DIE_AT(&e->lhs->where, "'%s' (%s-expr) is not a %sstruct or union (member %s)",
					type_ref_to_str(e->lhs->tree_type),
					e->lhs->f_str(),
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
	{
		decl *d_mem = struct_union_member_find(sue, spel,
				&e->bits.struct_mem.extra_off);

		if(!d_mem)
			DIE_AT(&e->where, "%s %s has no member named \"%s\"",
					sue_str(sue), sue->spel, spel);

		e->rhs->tree_type = (e->bits.struct_mem.d = d_mem)->ref;
	}

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
		cast->expr = addr = expr_new_addr(e->lhs);

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
	out_push_i(type_ref_new_INTPTR_T(), struct_offset(e)); /* integral offset */
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
			e->bits.struct_mem.d->spel);

	gen_str_indent++;
	print_expr(e->lhs);
	gen_str_indent--;
}

void fold_const_expr_struct(expr *e, consty *k)
{
	/* if lhs is NULL (or some pointer constant),
	 * const fold to struct offset, (obv. if !dot, which is taken care of in fold) */
	ASSERT_NOT_DOT();

	const_fold(e->lhs, k);

	switch(k->type){
		case CONST_NO:
		case CONST_NEED_ADDR:
		case CONST_STRK:
			k->type = CONST_NO;
			break;

		case CONST_ADDR:
			k->type = CONST_NEED_ADDR; /* not constant unless addressed e.g. &a->b */
			/* don't touch k->bits.addr info */

			/* obviously we offset this */
			k->offset = struct_offset(e);
			break;

		case CONST_VAL:
			k->type = CONST_NEED_ADDR; /* e.g. &((A *)0)->b */

			/* convert the val to a memaddr */
			/* read iv.val before we clobber it */
			k->bits.addr.bits.memaddr = k->bits.iv.val + struct_offset(e);
			k->offset = 0;

			k->bits.addr.is_lbl = 0;
			break;
	}
}

void mutate_expr_struct(expr *e)
{
	e->f_lea = gen_expr_struct_lea;
	e->f_const_fold = fold_const_expr_struct;
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
