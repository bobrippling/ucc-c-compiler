#include <assert.h>

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
				type_to_str(e->lhs->tree_type),
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

	/* pointer to struct - skip the pointer type and pull the quals
	 * off the struct type */
	struct_qual = type_qual(
			ptr_expect
			? type_dereference_decay(e->lhs->tree_type)
			: e->lhs->tree_type);

	/* pull qualifiers from the struct to the member */
	e->tree_type = type_qualify(e->bits.struct_mem.d->ref, struct_qual);
}

struct_union_enum_st *expr_struct_sutype(const expr *e)
{
	type *ty = e->lhs->tree_type;

	if(!e->expr_is_st_dot)
		ty = type_is_ptr(ty);

	return type_is_s_or_u(ty);
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
					octx, off, d->bits.var.struct_offset_bitfield, w);

			out_comment(octx, "struct bitfield lea");
		}
	}

	return off;
}

irval *gen_ir_expr_struct(const expr *e, irctx *ctx)
{
	const int ptr_expect = !e->expr_is_st_dot;
	decl *const d = e->bits.struct_mem.d;
	irval *struct_exp;
	const unsigned off = ctx->curval++;
	unsigned idx;
	int found = 0;
	struct_union_enum_st *su = type_is_s_or_u(
			ptr_expect
			? type_is_ptr(e->lhs->tree_type)
			: e->lhs->tree_type);

	assert(su && "no struct type");
	if(su->primitive == type_union){
#warning todo: union
		ICE("TODO: union");
	}

	found = irtype_struct_decl_index(su, d, &idx);
	assert(found && "couldn't find struct member index");

	struct_exp = gen_ir_expr(e->lhs, ctx);

	printf("$%u = elem %s, i4 %u\n", off, irval_str(struct_exp), idx);

	if(d->bits.var.field_width){
		/* bitfield loading handled by expr_cast (lval2rval),
		 * special cased, since we can't deref here as we're an lvalue */
	}

	if(e->bits.struct_mem.extra_off){
#warning extra_off
		ICE("TODO: struct extra_off");
	}

	return irval_from_id(off);
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
			k->type = CONST_NO;
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
			k->type = CONST_NEED_ADDR; /* e.g. &((A *)0)->b */

			/* convert the val to a memaddr */
			/* read num.val before we clobber it */
			k->bits.addr.bits.memaddr = k->bits.num.val.i + struct_offset(e);
			k->offset = 0;

			k->bits.addr.is_lbl = 0;
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
