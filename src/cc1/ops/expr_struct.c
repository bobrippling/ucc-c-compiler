#include "ops.h"
#include "expr_struct.h"
#include "../sue.h"
#include "../out/asm.h"
#include "../type_is.h"
#include "../type_nav.h"

#define struct_offset(e) (                            \
	 (e)->bits.struct_mem.d->bits.var.struct_offset +   \
	 (e)->bits.struct_mem.extra_off                     \
	 )

const char *str_expr_struct()
{
	return "member-access";
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
	enum type_qualifier struct_qual;
	attribute *struct_attr;
	type *struct_type;

	if(ptr_expect)
		FOLD_EXPR(e->lhs, stab);
	else
		fold_expr_nodecay(e->lhs, stab);

	/* don't fold the rhs - just a member name */
	if(e->rhs){
		UCC_ASSERT(expr_kind(e->rhs, identifier),
				"struct/union member not identifier (%s)", e->rhs->f_str());

		UCC_ASSERT(!e->bits.struct_mem.d, "already have a struct-member");

		spel = e->rhs->bits.ident.bits.ident.spel;
	}else{
		UCC_ASSERT(e->bits.struct_mem.d, "no member specified already?");
		spel = NULL;
	}

	/* we access a struct, of the right ptr depth */
	{
		type *r = e->lhs->tree_type;

		if(ptr_expect){
			type *rtest = type_is(r, type_ptr);

			if(!rtest && !(rtest = type_is(r, type_array)))
				goto err;

			r = rtest->ref; /* safe - rtest is an array */
		}

		if(!(sue = type_is_s_or_u(r))){
err:
			die_at(&e->lhs->where, "'%s' (%s-expr) is not a %sstruct or union (member %s)",
					type_to_str(e->lhs->tree_type),
					expr_str_friendly(e->lhs),
					ptr_expect ? "pointer to " : "",
					spel);
		}
	}

	if(!sue_is_complete(sue)){
		fold_had_error = 1;

		warn_at_print_error(
				&e->lhs->where, "%s incomplete type (%s)",
				ptr_expect
					? "dereferencing pointer to"
					: "accessing member of",
				type_to_str(e->lhs->tree_type));

		note_at(&sue->where, "forward declared here");

		e->tree_type = type_nav_btype(cc1_type_nav, type_int);
		return;
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

	/* pointer to struct - skip the pointer type and
	 * pull the quals and attrs off the struct type */
	struct_type = ptr_expect
			? type_dereference_decay(e->lhs->tree_type)
			: e->lhs->tree_type;

	struct_qual = type_qual(struct_type);
	struct_attr = type_get_attrs_toplvl(struct_type);

	/* pull qualifiers and attributes from the struct to the member */
	e->tree_type = type_attributed(
			type_qualify(
				e->bits.struct_mem.d->ref,
				struct_qual),
			struct_attr);
}

const out_val *gen_expr_struct(const expr *e, out_ctx *octx)
{
	const out_val *struct_exp, *off;

	/* cast for void* arithmetic */

	struct_exp = out_change_type(
			octx,
			gen_expr(e->lhs, octx),
			type_ptr_to(type_nav_btype(cc1_type_nav, type_void)));

	off =
		out_op(
				octx, op_plus,
				struct_exp,
				out_new_l(
					octx,
					type_nav_btype(cc1_type_nav, type_intptr_t),
					struct_offset(e)));

	if(fopt_mode & FOPT_VERBOSE_ASM)
		out_comment(octx, "struct member %s", e->bits.struct_mem.d->spel);


	{
		decl *d = e->bits.struct_mem.d;

		off = out_change_type(octx, off, type_ptr_to(d->ref));

		/* set if we're a bitfield - out_deref() and out_store()
		 * i.e. read + write then handle this
		 */
		if(d->bits.var.field_width){
			unsigned w = const_fold_val_i(d->bits.var.field_width);

			off = out_set_bitfield(
					octx,
					off,
					d->bits.var.struct_offset_bitfield,
					w,
					d->bits.var.bitfield_master_ty);

			out_comment(octx, "struct bitfield lea");
		}
	}

	return off;
}

void dump_expr_struct(const expr *e, dump *ctx)
{
	decl *mem = e->bits.struct_mem.d;

	dump_desc_expr_newline(ctx, "member-access", e, 0);

	dump_printf_indent(ctx, 0, " %s%s\n",
			e->expr_is_st_dot ? "." : "->",
			mem->spel);

	dump_inc(ctx);
	dump_expr(e->lhs, ctx);
	dump_dec(ctx);
}

static void fold_const_expr_struct(expr *e, consty *k)
{
	/* if lhs is NULL (or some pointer constant),
	 * const fold to struct offset, (obv. if !dot, which is taken care of in fold) */
	const_fold(e->lhs, k);

	switch(k->type){
		case CONST_NO:
		case CONST_STRK:
			CONST_FOLD_NO(k, e);
			break;

		case CONST_NEED_ADDR:
		case CONST_ADDR:
			/* a.b == (&a)->b, i.e. a.b wants a CONST_NEED_ADDR
			 * a->b wants a CONST_ADDR
			 */
			if(k->type == (e->expr_is_st_dot ? CONST_NEED_ADDR : CONST_ADDR)){
				/* not constant unless addressed e.g. &a->b (unless array/func) */
				k->type = CONST_ADDR_OR_NEED(e->bits.struct_mem.d);
				/* don't touch k->bits.addr info */

				/* obviously we offset this */
				k->offset += struct_offset(e);
			}else{
				k->type = CONST_NO;
			}
			break;

		case CONST_NUM:
			ICE("const expr struct - address expected");
			break;
	}
}

static enum lvalue_kind struct_is_lval(expr *e)
{
	if(e->expr_is_st_dot){
		/* we're only an lvalue if our subexpression is a
		 * non-internal/C-standard lvalue.
		 *
		 * unless we're being checked internally, in which
		 * case we want lval2rval decay - hence yes
		 */
		return expr_is_lval(e->lhs);
	}else{
		return LVALUE_USER_ASSIGNABLE;
	}
}

void mutate_expr_struct(expr *e)
{
	e->f_const_fold = fold_const_expr_struct;
	e->f_islval = struct_is_lval;

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

const out_val *gen_expr_style_struct(const expr *e, out_ctx *octx)
{
	IGNORE_PRINTGEN(gen_expr(e->lhs, octx));
	stylef("->%s", e->bits.struct_mem.d->spel);
	return NULL;
}
