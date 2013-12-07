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

	if(e->rhs){
		UCC_ASSERT(expr_kind(e->rhs, identifier),
				"struct/union member not identifier (%s)", e->rhs->f_str());

		UCC_ASSERT(!e->bits.struct_mem.d, "already have a struct-member");

		spel = e->rhs->bits.ident.spel;
	}else{
		UCC_ASSERT(e->bits.struct_mem.d, "no member specified already?");
		spel = NULL;
	}

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
			die_at(&e->lhs->where, "'%s' (%s-expr) is not a %sstruct or union (member %s)",
					type_ref_to_str(e->lhs->tree_type),
					e->lhs->f_str(),
					ptr_expect ? "pointer to " : "",
					spel);
		}
	}

	if(!sue_complete(sue)){
		char wbuf[WHERE_BUF_SIZ];

		die_at(&e->lhs->where, "%s incomplete type (%s)\n"
				"%s: note: forward declared here",
				ptr_expect
					? "dereferencing pointer to"
					: "accessing member of",
				type_ref_to_str(e->lhs->tree_type),
				where_str_r(wbuf, &sue->where));
	}

	if(spel){
		/* found the struct, find the member */
		decl *d_mem = struct_union_member_find(sue, spel,
				&e->bits.struct_mem.extra_off, NULL);

		if(!d_mem)
			die_at(&e->where, "%s %s has no member named \"%s\"",
					sue_str(sue), sue->spel, spel);

		e->rhs->tree_type = (e->bits.struct_mem.d = d_mem)->ref;
	}/* else already have the member */

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

		addr = expr_new_addr(e->lhs);
		cast = expr_new_cast(addr, type_ref_cached_VOID_PTR(), 1);

		e->lhs = cast;
		e->expr_is_st_dot = 0;

		FOLD_EXPR(e->lhs, stab);
	}

	/* pull qualifiers from the struct to the member */
	{
		enum type_qualifier addon = type_ref_qual(e->lhs->tree_type);

		e->tree_type = type_ref_new_cast_add(e->bits.struct_mem.d->ref, addon);
	}
}

static void gen_expr_struct_lea(expr *e)
{
	ASSERT_NOT_DOT();

	gen_expr(e->lhs);

	out_change_type(type_ref_cached_VOID_PTR()); /* cast for void* arithmetic */
	out_push_i(type_ref_cached_INTPTR_T(), struct_offset(e)); /* integral offset */
	out_op(op_plus);

	{
		decl *d = e->bits.struct_mem.d;

		out_change_type(type_ref_ptr_depth_inc(d->ref));

		/* set if we're a bitfield - out_deref() and out_store()
		 * i.e. read + write then handle this
		 */
		if(d->field_width){
			unsigned w = const_fold_val(d->field_width);
			out_set_bitfield(d->struct_offset_bitfield, w);
			out_comment("struct bitfield lea");
		}
	}
}

void gen_expr_struct(expr *e)
{
	ASSERT_NOT_DOT();

	gen_expr_struct_lea(e);

	out_deref();

	out_comment("val from struct/union");
}

void gen_expr_str_struct(expr *e)
{
	decl *mem = e->bits.struct_mem.d;

	idt_printf("struct/union member %s offset %d\n",
			mem->spel, struct_offset(e));

	if(mem->field_width)
		idt_printf("bitfield offset %u, width %u\n",
				mem->struct_offset_bitfield,
				(unsigned)const_fold_val(mem->field_width));

	gen_str_indent++;
	print_expr(e->lhs);
	gen_str_indent--;
}

static void fold_const_expr_struct(expr *e, consty *k)
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
			/* not constant unless addressed e.g. &a->b (unless array/func) */
			k->type = CONST_ADDR_OR_NEED(e->bits.struct_mem.d);
			/* don't touch k->bits.addr info */

			/* obviously we offset this */
			k->offset += struct_offset(e);
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
	/* unconditionally an lvalue */
	e->f_lea = gen_expr_struct_lea;

	e->f_const_fold = fold_const_expr_struct;

	/* zero out the union/rhs if we're mutating */
	e->bits.struct_mem.d = NULL;
	e->rhs = NULL;
}

expr *expr_new_struct(expr *sub, int dot, expr *ident)
{
	expr *e = expr_new_wrapper(struct);
	e->expr_is_st_dot = dot;
	e->lhs = sub;
	e->rhs = ident;
	return e;
}

expr *expr_new_struct_mem(expr *sub, int dot, decl *d)
{
	expr *e = expr_new_wrapper(struct);
	e->expr_is_st_dot = dot;
	e->lhs = sub;
	e->bits.struct_mem.d = d;
	return e;
}

void gen_expr_style_struct(expr *e)
{
	gen_expr(e->lhs);
	stylef("->%s", e->bits.struct_mem.d->spel);
}
